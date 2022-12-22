/**
  ******************************************************************************
  * @file    Audio/Audio_playback_and_record/Src/waverecorder.c 
  * @author  Original from MCD Application Team, this version was restructured by Sandra and Ro
  * @brief   I2S Audio recorder program.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "waverecorder.h" 
#include "string.h"

/* Private typedef -----------------------------------------------------------*/

/* struct for measuring offset*/
typedef struct {
  uint32_t fptr;
}Audio_BufferTypeDef;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

struct tc_ccm_mode_struct c;
struct tc_aes_key_sched_struct sched;

/* Initialize encryption key, header and nonce*/
/* Key is: "Sandra und Ro G4" in HEX*/
const uint8_t key[16] = {
		0x53, 0x61, 0x6e, 0x64, 0x72, 0x61, 0x20, 0x75,
		0x6e, 0x64, 0x20, 0x52, 0x6f, 0x20, 0x47, 0x34
	};
uint8_t nonce[NONCE_LEN] = {
	0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0,
	0xa1, 0xa2, 0xa3, 0xa4, 0xa5
};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};

/* Nonce and random number*/
uint8_t *pnonce = nonce;
uint32_t RandomNumber = 0;
uint32_t* pRandomNumber = &RandomNumber;

/* array for encrypted blocks*/
uint8_t ciphertext[TC_CCM_MAX_CT_SIZE+M_LEN8];

/* */
uint8_t result = 0;

uint8_t uart_return = 0;

/* Wave recorded counter.*/
__IO uint32_t WaveCounter = 0;

extern __IO uint32_t CmdIndex, LEDsState, TimeRecBase;
extern uint8_t RecBuffer;
extern uint8_t* pRecBuffer;
extern uint32_t pRecBufferOffset;
extern UART_HandleTypeDef huart5;

/* USB variable to check if USB connected or not */
extern MSC_ApplicationTypeDef AppliState;

/* Variable used to switch play from audio sample available on USB to recorded file */
uint32_t WaveRecStatus = 0; 

uint8_t pHeaderBuff[44];
uint16_t WrBuffer[WR_BUFFER_SIZE];

static uint16_t RecBuf[PCM_OUT_SIZE*2];/* PCM stereo samples are saved in RecBuf */
static uint16_t InternalBuffer[INTERNAL_BUFF_SIZE];
__IO uint32_t ITCounter = 0;
Audio_BufferTypeDef  BufferCtl;

/* Temporary data sample */
__IO uint32_t AUDIODataReady = 0, AUDIOBuffOffset = 0;

WAVE_FormatTypeDef WaveFormat;
FIL WavFile;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static uint32_t WavProcess_EncInit(uint32_t Freq, uint8_t *pHeader);
static uint32_t WavProcess_HeaderInit(uint8_t *pHeader, WAVE_FormatTypeDef *pWaveFormatStruct);
static uint32_t WavProcess_HeaderUpdate(uint8_t *pHeader, WAVE_FormatTypeDef *pWaveFormatStruct);

/* from https://stm32f4-discovery.net/2014/07/library-22-true-random-number-generator-stm32f4xx/ */
static uint32_t GenerateNonce(void);

/*  
  A single MEMS microphone MP45DT02 mounted on STM32F4-Discovery is connected 
  to the Inter-IC Sound (I2S) peripheral. The I2S is configured in master 
  receiver mode. In this mode, the I2S peripheral provides the clock to the MEMS 
  microphone through CLK_IN and acquires the data (Audio samples) from the MEMS 
  microphone through PDM_OUT.

  Data acquisition is performed in 16-bit PDM format and using I2S IT mode.
  
  In order to avoid data-loss, two buffers are used: 
   - When there are enough data to be transmitted to USB, there is a buffer reception
   switch. 
  
  To avoid data-loss:
  - IT ISR priority must be set at a higher priority than USB, this priority 
    order must be respected when managing other interrupts; 

  Note that a PDM Audio software decoding library provided in binary is used in 
  this application. For IAR EWARM toolchain, the library is labeled 
  "libPDMFilter_CM4_IAR.a".
*/

/**
  * @brief  Generate random Nonce
  * @param
  * @retval uint32_t random number
  */
static uint32_t GenerateNonce(void)
{
	/* initialize variable to return nonce*/
	uint32_t random_nonce = 0;

	/* Enable the RNG clock source*/
	RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;

	/* RNG Peripheral enable */
	RNG->CR |= RNG_CR_RNGEN;

	/* wait until the RNG is ready*/
	while((RNG->SR & RNG_SR_DRDY) == 0)
	{
		/* wait */
	}

	/* read the generated random number*/
	random_nonce = RNG->DR;

	/* Disable RNG peripheral */
	RNG->CR &= ~RNG_CR_RNGEN;

	/* Disable the RNG peripheral*/
	RCC->AHB2ENR &= ~RCC_AHB2ENR_RNGEN;

	return random_nonce;
}

/**
  * @brief  Start Audio recording.
  * @param  pBuf: pointer to a buffer
  *         wSize: Buffer size
  * @retval None
  */
uint8_t WaveRecorderStart(uint16_t* pBuf, uint32_t wSize)
{
  return (BSP_AUDIO_IN_Record(pBuf, wSize));
}

/**
  * @brief  Stop Audio recording.
  * @param  None
  * @retval None
  */
uint32_t WaveRecorderStop(void)
{
	return BSP_AUDIO_IN_Stop();
}

/**
  * @brief  Update the recorded data. 
  * @param  None
  * @retval None
  */
void WaveRecorderProcess(void)
{     
	/* Current size of the recorded buffer */
	uint32_t byteswritten = 0;

	/* set recorder buffer to 0 for new recording*/
	if (NULL != pRecBuffer)
	{
		memset((void*)pRecBuffer,0,SIZE_OF_RECORD_BUFFER);
	}
	else
	{
		Error_Handler();
	}
	/* Turn OFF all LEDs */
	LEDsState = LEDS_OFF;

	/* Initialize header file */
	if((0 != WavProcess_EncInit(DEFAULT_AUDIO_IN_FREQ, pHeaderBuff)) && NULL == pHeaderBuff)
	{
		Error_Handler();
	}

	/* Increment the Wave counter */
	BufferCtl.fptr = byteswritten;

//	BufferCtl.offset = BUFFER_OFFSET_NONE;
	if (AUDIO_OK != BSP_AUDIO_IN_Init(DEFAULT_AUDIO_IN_FREQ, DEFAULT_AUDIO_IN_BIT_RESOLUTION, DEFAULT_AUDIO_IN_CHANNEL_NBR))
	{
		Error_Handler();
	}
	if(AUDIO_OK != BSP_AUDIO_IN_Record((uint16_t*)&InternalBuffer[0], INTERNAL_BUFF_SIZE))
	{
		Error_Handler();
	}

	/* Reset the time recording base variable */
	TimeRecBase = 0;

	/* Reset the interrupt counter*/
	ITCounter = 0;

	/* Toggle orange LED*/
	LEDsState = LED3_TOGGLE;

	while(AppliState != APPLICATION_IDLE)
	{
		/* Wait for the recording time */
		if (TimeRecBase <= DEFAULT_TIME_REC)
		{
			/* Check if there are Data to write in Buffer */
			if(AUDIODataReady == 1)
			{
				if (NULL != pRecBuffer || NULL != WrBuffer)
				{
					/* write data in Buffer*/
					memcpy((void*)(pRecBuffer+BufferCtl.fptr),(const void*)((uint8_t*)WrBuffer+AUDIOBuffOffset),WR_BUFFER_SIZE);
				}
				else
				{
					Error_Handler();
				}
				/* update buffer offset */
				BufferCtl.fptr += WR_BUFFER_SIZE;

				/* set flag for data back to 0*/
				AUDIODataReady = 0;
			}

			/* User button pressed */
			if (CmdIndex != CMD_RECORD)
			{
				/* Stop Audio Recording */
				WaveRecorderStop();
				/* Switch Command Index to Stop */
				CmdIndex = CMD_STOP;
				/* Toggoling Green LED to signal Stop */
				LEDsState = LED4_TOGGLE;
				break;
			}
		}

		else /* End of recording time TIME_REC -> RecBuffer is full and can be encrypter and transmitted*/
		{
			/* Stop Audio Recording */
			WaveRecorderStop();

			/* Change Command Index to Stop */
			CmdIndex = CMD_STOP;

			/* Turn on Blue LED to signal transmission */
			LEDsState = LED6_TOGGLE;

			/* set flag for data back to 0*/
			AUDIODataReady = 0;
      
			//set encryption Key
			tc_aes128_set_encrypt_key(&sched, (const uint8_t *)key);

			/* Parse the wav file header and extract required information */
			if (0 != WavProcess_HeaderUpdate(pHeaderBuff, &WaveFormat))
			{
				Error_Handler();
			}

			/* Write the header in the RecBuffer*/
			if (NULL != pRecBuffer || NULL != pHeaderBuff)
			{
				memcpy((void*)(pRecBuffer),(const void*)pHeaderBuff,44);
			}

			/* Encryp data in 4088 bit blocks*/
			for (uint8_t i=0; i<NUMBER_OF_AES_BLOCKS; i++)
			{
				/* generate random nonce for every transmission*/
				for (uint8_t j=0; j<NONCE_LEN; (j+=4))
				{
					/* set first 12 bytes of nonce with random numbers*/
					if (12 != j )
					{
						RandomNumber = GenerateNonce();
						if (NULL != pnonce || NULL != pRandomNumber)
						{
							memcpy((void*)(pnonce+j),(const void*)pRandomNumber,4);
						}
					}
					/* set 13th byte of nonce with a random number*/
					else
					{
						RandomNumber = GenerateNonce();
						if (NULL != pnonce || NULL != pRandomNumber)
						{
							memcpy((void*)(pnonce+j),(const void*)pRandomNumber,1);
						{
					}
				}

				/* config encryption parameter*/
				result = (uint8_t)tc_ccm_config(&c, &sched, nonce, NONCE_LEN, M_LEN8);
				if (TC_CRYPTO_FAIL == result)
				{
					Error_Handler();
				}

				/* encrypt block*/
				if (NULL != pRecBuffer)
				{
					result = (uint8_t)tc_ccm_generation_encryption(ciphertext,
																   TC_CCM_MAX_CT_SIZE+M_LEN8,
																   hdr,
																   HEADER_LEN,
																   pRecBuffer+i*TC_CCM_MAX_CT_SIZE,
																   TC_CCM_MAX_CT_SIZE,
																   &c);
				{
				if (TC_CRYPTO_FAIL == result)
				{
					Error_Handler();
				}
				/* Transmit cipher text over uart, check for HAL_ERROR & HAL_BUSY, HAL_TIMEOUT does not apply*/
				uart_return = (uint8_t)HAL_UART_Transmit(&huart5, hdr, HEADER_LEN, 15000);
				if (HAL_ERROR == uart_return)
				{
					Error_Handler();
				}

				/* if ressource is busy, wait 0.5 seconds and retry the transmission, else Error_Handler()*/
				else if (HAL_BUSY == uart_return)
				{
					HAL_Delay(500);
					uart_return = (uint8_t)HAL_UART_Transmit(&huart5, hdr, HEADER_LEN, 15000);
					if (HAL_BUSY == uart_return)
					{
						Error_Handler();
					}
				}
				uart_return = (uint8_t)HAL_UART_Transmit(&huart5, nonce, NONCE_LEN, 15000);
				if (HAL_ERROR == uart_return)
					{
						Error_Handler();
					}
				/* if ressource is busy, wait 0.5 seconds and retry the transmission, else Error_Handler()*/
				else if (HAL_BUSY == uart_return)
				{
					HAL_Delay(500);
					uart_return = (uint8_t)HAL_UART_Transmit(&huart5, nonce, NONCE_LEN, 15000);
					if (HAL_BUSY == uart_return)
					{
						Error_Handler();
					}
				}
				uart_return = (uint8_t)HAL_UART_Transmit(&huart5, ciphertext, TC_CCM_MAX_CT_SIZE+M_LEN8, 15000);
				if (HAL_ERROR == uart_return)
					{
						Error_Handler();
					}
				/* if ressource is busy, wait 0.5 seconds and retry the transmission, else Error_Handler()*/
				else if (HAL_BUSY == uart_return)
				{
					HAL_Delay(500);
					uart_return = (uint8_t)HAL_UART_Transmit(&huart5, ciphertext, TC_CCM_MAX_CT_SIZE+M_LEN8, 15000);
					if (HAL_BUSY == uart_return)
					{
						Error_Handler();
					}
				}
			}
			break;
		}
	}

	/* Change Command Index to Stop*/
	CmdIndex = CMD_STOP;
}

/**
  * @brief  Calculates the remaining file size and new position of the pointer.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
	/* PDM to PCM data convert */
	if(AUDIO_OK != BSP_AUDIO_IN_PDMToPCM((uint16_t*)&InternalBuffer[INTERNAL_BUFF_SIZE/2], (uint16_t*)&RecBuf[0]))
		{
			Error_Handler();
		}

	/* Copy PCM data in internal buffer */
	memcpy((uint16_t*)&WrBuffer[ITCounter * (PCM_OUT_SIZE*2)], RecBuf, PCM_OUT_SIZE*4);

	  /* Check, if first half of WrBuffer full is*/
	if(ITCounter == (WR_BUFFER_SIZE/(PCM_OUT_SIZE*4))-1)
	{
		AUDIODataReady = 1;
		AUDIOBuffOffset = 0;
		ITCounter++;

	}
	  /* Check, if second half of WrBuffer full is*/
	else if(ITCounter == (WR_BUFFER_SIZE/(PCM_OUT_SIZE*2))-1)
	{
		AUDIODataReady = 1;
		AUDIOBuffOffset = WR_BUFFER_SIZE/2;
		ITCounter = 0;
	}
	else
	{
		ITCounter++;
	}
}

/**
  * @brief  Manages the DMA Half Transfer complete interrupt.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
  /* PDM to PCM data convert */
  if(AUDIO_OK != BSP_AUDIO_IN_PDMToPCM((uint16_t*)&InternalBuffer[0], (uint16_t*)&RecBuf[0]))
  {
	  Error_Handler();
  }

  /* Copy PCM data in internal buffer */
  memcpy((uint16_t*)&WrBuffer[ITCounter * (PCM_OUT_SIZE*2)], RecBuf, PCM_OUT_SIZE*4);

  /* Check, if first half of WrBuffer full is*/
  if(ITCounter == (WR_BUFFER_SIZE/(PCM_OUT_SIZE*4))-1)
  {
    AUDIODataReady = 1;
    AUDIOBuffOffset = 0;
    ITCounter++;
  }
  /* Check, if second half of WrBuffer full is*/
  else if(ITCounter == (WR_BUFFER_SIZE/(PCM_OUT_SIZE*2))-1)
  {
    AUDIODataReady = 1;
    AUDIOBuffOffset = WR_BUFFER_SIZE/2;
    ITCounter = 0;
  }
  else
  {
    ITCounter++;
  }
}

/**
  * @brief  Encoder initialization.
  * @param  Freq: Sampling frequency.
  * @param  pHeader: Pointer to the WAV file header to be written.  
  * @retval 0 if success, !0 else.
  */
static uint32_t WavProcess_EncInit(uint32_t Freq, uint8_t* pHeader)
{  
  /* Initialize the encoder structure */
  WaveFormat.SampleRate = Freq;        /* Audio sampling frequency */
  WaveFormat.NbrChannels = 2;          /* Number of channels: 1:Mono or 2:Stereo */
  WaveFormat.BitPerSample = 16;        /* Number of bits per sample (16, 24 or 32) */
  WaveFormat.FileSize = 0x001D4C00;    /* Total length of useful audio data (payload) */
  WaveFormat.SubChunk1Size = 44;       /* The file header chunk size */
  WaveFormat.ByteRate = (WaveFormat.SampleRate * \
                        (WaveFormat.BitPerSample/8) * \
                         WaveFormat.NbrChannels);     /* Number of bytes per second  (sample rate * block align)  */
  WaveFormat.BlockAlign = WaveFormat.NbrChannels * \
                         (WaveFormat.BitPerSample/8); /* channels * bits/sample / 8 */
  
  /* Parse the wav file header and extract required information */
  if(WavProcess_HeaderInit(pHeader, &WaveFormat))
  {
    return 1;
  }
  return 0;
}

/**
  * @brief  Initialize the wave header file
  * @param  pHeader: Header Buffer to be filled
  * @param  pWaveFormatStruct: Pointer to the wave structure to be filled.
  * @retval 0 if passed, !0 if failed.
  */
static uint32_t WavProcess_HeaderInit(uint8_t* pHeader, WAVE_FormatTypeDef* pWaveFormatStruct)
{
  /* Write chunkID, must be 'RIFF'  ------------------------------------------*/
  pHeader[0] = 'R';
  pHeader[1] = 'I';
  pHeader[2] = 'F';
  pHeader[3] = 'F';
  
  /* Write the file length ----------------------------------------------------*/
  /* The sampling time: this value will be be written back at the end of the 
     recording operation.  Example: 661500 Bytes = 0x000A17FC, byte[7]=0x00, byte[4]=0xFC */
  pHeader[4] = 0x00;
  pHeader[5] = 0x4C;
  pHeader[6] = 0x1D;
  pHeader[7] = 0x00;
  
  /* Write the file format, must be 'WAVE' -----------------------------------*/
  pHeader[8]  = 'W';
  pHeader[9]  = 'A';
  pHeader[10] = 'V';
  pHeader[11] = 'E';
  
  /* Write the format chunk, must be'fmt ' -----------------------------------*/
  pHeader[12]  = 'f';
  pHeader[13]  = 'm';
  pHeader[14]  = 't';
  pHeader[15]  = ' ';
  
  /* Write the length of the 'fmt' data, must be 0x10 ------------------------*/
  pHeader[16]  = 0x10;
  pHeader[17]  = 0x00;
  pHeader[18]  = 0x00;
  pHeader[19]  = 0x00;
  
  /* Write the audio format, must be 0x01 (PCM) ------------------------------*/
  pHeader[20]  = 0x01;
  pHeader[21]  = 0x00;
  
  /* Write the number of channels, ie. 0x01 (Mono) ---------------------------*/
  pHeader[22]  = pWaveFormatStruct->NbrChannels;
  pHeader[23]  = 0x00;
  
  /* Write the Sample Rate in Hz ---------------------------------------------*/
  /* Write Little Endian ie. 8000 = 0x00001F40 => byte[24]=0x40, byte[27]=0x00*/
  pHeader[24]  = (uint8_t)((pWaveFormatStruct->SampleRate & 0xFF));
  pHeader[25]  = (uint8_t)((pWaveFormatStruct->SampleRate >> 8) & 0xFF);
  pHeader[26]  = (uint8_t)((pWaveFormatStruct->SampleRate >> 16) & 0xFF);
  pHeader[27]  = (uint8_t)((pWaveFormatStruct->SampleRate >> 24) & 0xFF);
  
  /* Write the Byte Rate -----------------------------------------------------*/
  pHeader[28]  = (uint8_t)((pWaveFormatStruct->ByteRate & 0xFF));
  pHeader[29]  = (uint8_t)((pWaveFormatStruct->ByteRate >> 8) & 0xFF);
  pHeader[30]  = (uint8_t)((pWaveFormatStruct->ByteRate >> 16) & 0xFF);
  pHeader[31]  = (uint8_t)((pWaveFormatStruct->ByteRate >> 24) & 0xFF);
  
  /* Write the block alignment -----------------------------------------------*/
  pHeader[32]  = pWaveFormatStruct->BlockAlign;
  pHeader[33]  = 0x00;
  
  /* Write the number of bits per sample -------------------------------------*/
  pHeader[34]  = pWaveFormatStruct->BitPerSample;
  pHeader[35]  = 0x00;
  
  /* Write the Data chunk, must be 'data' ------------------------------------*/
  pHeader[36]  = 'd';
  pHeader[37]  = 'a';
  pHeader[38]  = 't';
  pHeader[39]  = 'a';
  
  /* Write the number of sample data -----------------------------------------*/
  /* This variable will be written back at the end of the recording operation */
  pHeader[40]  = 0x00;
  pHeader[41]  = 0x4C;
  pHeader[42]  = 0x1D;
  pHeader[43]  = 0x00;
  
  /* Return 0 if all operations are OK */
  return 0;
}

/**
  * @brief  Initialize the wave header file
  * @param  pHeader: Header Buffer to be filled
  * @param  pWaveFormatStruct: Pointer to the wave structure to be filled.
  * @retval 0 if passed, !0 if failed.
  */
static uint32_t WavProcess_HeaderUpdate(uint8_t* pHeader, WAVE_FormatTypeDef* pWaveFormatStruct)
{
  /* Write the file length ----------------------------------------------------*/
  /* The sampling time: this value will be be written back at the end of the 
     recording operation.  Example: 661500 Bytes = 0x000A17FC, byte[7]=0x00, byte[4]=0xFC */
  pHeader[4] = (uint8_t)(BufferCtl.fptr);
  pHeader[5] = (uint8_t)(BufferCtl.fptr >> 8);
  pHeader[6] = (uint8_t)(BufferCtl.fptr >> 16);
  pHeader[7] = (uint8_t)(BufferCtl.fptr >> 24);
  /* Write the number of sample data -----------------------------------------*/
  /* This variable will be written back at the end of the recording operation */
  BufferCtl.fptr -=44;
  pHeader[40] = (uint8_t)(BufferCtl.fptr); 
  pHeader[41] = (uint8_t)(BufferCtl.fptr >> 8);
  pHeader[42] = (uint8_t)(BufferCtl.fptr >> 16);
  pHeader[43] = (uint8_t)(BufferCtl.fptr >> 24); 
  /* Return 0 if all operations are OK */
  return 0;
}
