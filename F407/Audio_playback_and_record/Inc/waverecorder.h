/**
  ******************************************************************************
  * @file    Audio/Audio_playback_and_record/Inc/waverecorder.h
  * @author  MCD Application Team
  * @brief   Header for waverecorder.c module.
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
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WAVERECORDER_H
#define __WAVERECORDER_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "main.h"


/* Exported types ------------------------------------------------------------*/
/* Exported Defines ----------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Defines for the Audio recording process */
#define WR_BUFFER_SIZE           1024  /*  More the size is higher, the recorded quality is better */
#define DEFAULT_TIME_REC         1000  /* Recording time in millisecond (Systick Time Base*TIME_REC= 1ms*750)
                                        (default: 0.75s) */
#define SIZE_OF_RECORD_BUFFER    65408 /* Size of internal buffer, where the recording is saved in bytes*/
#define NUMBER_OF_AES_BLOCKS	 16  /* (SIZE_OF_RECORD_BUFFER/TC_AES_BLOCK_SIZE) = (65408/4088) = 16*/

// AES CCM Mode constants
// https://github.com/intel/tinycrypt/blob/master/tests/test_ccm_mode.c
#define TC_CCM_MAX_CT_SIZE 4088
#define TC_CCM_MAX_PT_SIZE 44
#define TC_PASS 0
#define TC_FAIL 1
#define M_LEN8 8
#define NONCE_LEN 13
#define HEADER_LEN 8

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
uint32_t  WaveRecorderInit(uint32_t AudioFreq, uint32_t BitRes, uint32_t ChnlNbr);
uint8_t   WaveRecorderStart(uint16_t* pBuf, uint32_t wSize);
uint32_t  WaveRecorderStop(void);
void      WaveRecorderProcess(void);
void      WaveRecorderProcessingAudioBuffer(void);
#endif /* __WAVERECORDER_H */
