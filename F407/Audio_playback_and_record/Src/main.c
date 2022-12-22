/**
  ******************************************************************************
  * @file    Audio/Audio_playback_and_record/Src/main.c 
  * @author  Original from MCD Application Team, this version was restructured by Sandra and Ro
  * @brief   Main program body
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

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef hTimLed;
TIM_OC_InitTypeDef sConfigLed;
UART_HandleTypeDef huart5;

/* Buffer for audio recording*/
uint8_t RecBuffer[SIZE_OF_RECORD_BUFFER];
uint8_t* pRecBuffer = RecBuffer;

/* debounce time measurement, in ms*/
uint32_t LastDebounceTime = 0;
uint32_t DebounceTime = 100;

/* Capture Compare Register Value.
   Defined as external in stm32f4xx_it.c file */
__IO uint16_t CCR1Val = 16826;              
                                            
/* LED State (Toggle or OFF)*/
__IO uint32_t LEDsState;

/* Save MEMS ID */
uint8_t MemsID = 0; 

__IO uint32_t CmdIndex = CMD_RECORD;

MSC_ApplicationTypeDef AppliState = APPLICATION_START;

/* Private function prototypes -----------------------------------------------*/
static void TIM_LED_Config(void);
static void SystemClock_Config(void);
static void COMMAND_AudioExecuteApplication(void);
static void MX_UART5_Init(void);

int main(void)
{
	/* STM32F4xx HAL library initialization:
	 - Configure the Flash prefetch, instruction and Data caches
	 - Configure the Systick to generate an interrupt each 1 msec
	 - Set NVIC Group Priority to 4
	 - Global MSP (MCU Support Package) initialization
	*/
	HAL_Init();

	/* Configure LED3, LED4, LED5 and LED6 */
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED5);
	BSP_LED_Init(LED6);

	/* Configure the system clock to 168 MHz */
	SystemClock_Config();

	/* Initialize MEMS Accelerometer mounted on STM32F4-Discovery board */
	if(BSP_ACCELERO_Init() != ACCELERO_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}
  
	MemsID = BSP_ACCELERO_ReadID();
  
	/* Configure TIM4 Peripheral to manage LEDs lighting */
	TIM_LED_Config();
  
	/* Configure UART5 Peripheral to manage UART transmission*/
	MX_UART5_Init();

	/* Turn OFF all LEDs */
	LEDsState = LEDS_OFF;

	/* Configure USER Button */
	BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
  
    /* Run Application (Blocking mode)*/
    while (1)
    {
        COMMAND_AudioExecuteApplication();
    }
}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{
	huart5.Instance = UART5;
	huart5.Init.BaudRate = 115200;
	huart5.Init.WordLength = UART_WORDLENGTH_8B;
	huart5.Init.StopBits = UART_STOPBITS_1;
	huart5.Init.Parity = UART_PARITY_NONE;
	huart5.Init.Mode = UART_MODE_TX_RX;
	huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart5.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart5) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
  * @brief  COMMAND_AudioExecuteApplication.
  * @param  None
  * @retval None
  */
static void COMMAND_AudioExecuteApplication(void)
{
	/* Execute the command switch the command index */
	switch (CmdIndex)
	{
		/* Start Recording in RecBuffer */
		case CMD_RECORD:
			WaveRecorderProcess();
			break;

			/* Go into idle state, if not recording*/
		case CMD_STOP:
			if (LEDsState != LED4_TOGGLE)
			{
				LEDsState = LED4_TOGGLE;
			}
			break;

		default:
			break;
	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();
  
	/* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 50;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}

}

/**
  * @brief  Configures TIM4 Peripheral for LEDs lighting.
  * @param  None
  * @retval None
  */
static void TIM_LED_Config(void)
{
	uint16_t prescalervalue = 0;
	uint32_t tmpvalue = 0;

	/* TIM4 clock enable */
	__HAL_RCC_TIM4_CLK_ENABLE();

	/* Enable the TIM4 global Interrupt */
	HAL_NVIC_SetPriority(TIM4_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(TIM4_IRQn);
  
	/* -----------------------------------------------------------------------
  	  TIM4 Configuration: Output Compare Timing Mode:
    	To get TIM4 counter clock at 550 KHz, the prescaler is computed as follows:
    	Prescaler = (TIM4CLK / TIM4 counter clock) - 1
    	Prescaler = ((f(APB1) * 2) /550 KHz) - 1
  
    	CC update rate = TIM4 counter clock / CCR_Val = 32.687 Hz
    	==> Toggling frequency = 16.343 Hz
  ----------------------------------------------------------------------- */
  
  /* Compute the prescaler value */
	tmpvalue = HAL_RCC_GetPCLK1Freq();
	prescalervalue = (uint16_t) ((tmpvalue * 2) / 550000) - 1;
  
	/* Time base configuration */
	hTimLed.Instance = TIM4;
	hTimLed.Init.Period = 65535;
	hTimLed.Init.Prescaler = prescalervalue;
	hTimLed.Init.ClockDivision = 0;
	hTimLed.Init.CounterMode = TIM_COUNTERMODE_UP;
	if(HAL_TIM_OC_Init(&hTimLed) != HAL_OK)
	{
		/* Start Error */
		Error_Handler();
	}
  
	/* Output Compare Timing Mode configuration: Channel1 */
	sConfigLed.OCMode = TIM_OCMODE_TIMING;
	sConfigLed.OCIdleState = TIM_OCIDLESTATE_SET;
	sConfigLed.Pulse = CCR1Val;
	sConfigLed.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigLed.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigLed.OCFastMode = TIM_OCFAST_ENABLE;
	sConfigLed.OCNIdleState = TIM_OCNIDLESTATE_SET;
  
	/* Initialize the TIM4 Channel1 with the structure above */
	if(HAL_TIM_OC_ConfigChannel(&hTimLed, &sConfigLed, TIM_CHANNEL_1) != HAL_OK)
	{
		/* Start Error */
		Error_Handler();
	}

	/* Start the Output Compare */
	if(HAL_TIM_OC_Start_IT(&hTimLed, TIM_CHANNEL_1) != HAL_OK)
	{
		/* Start Error */
		Error_Handler();
	}
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
	/* Turn red LED on */
	BSP_LED_On(LED5);
	/* stays in this loop, until user resets STM32F407 Discovery */
	while(1)
	{
	}
}

/**
  * @brief  Output Compare callback in non blocking mode 
  * @param  htim : TIM OC handle
  * @retval None
  */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
	uint32_t capture = 0;

	/* Toggle orange LED*/
	if (LEDsState == LED3_TOGGLE)
	{
		BSP_LED_Toggle(LED3);
		BSP_LED_Off(LED6);
		BSP_LED_Off(LED4);
	}

	/* Toggle green LED*/
	else if (LEDsState == LED4_TOGGLE)
	{
		BSP_LED_Toggle(LED4);
		BSP_LED_Off(LED6);
		BSP_LED_Off(LED3);
	}

	/* Toggle Blue LED*/
	else if (LEDsState == LED6_TOGGLE)
	{
		/* Toggling LED6 */
		BSP_LED_Off(LED3);
		BSP_LED_Off(LED4);
		BSP_LED_Toggle(LED6);
	}
	else if (LEDsState == LEDS_OFF)
	{
		/* Turn OFF all LEDs */
		BSP_LED_Off(LED3);
		BSP_LED_Off(LED4);
		BSP_LED_Off(LED5);
		BSP_LED_Off(LED6);
	}
	/* Get the TIM4 Input Capture 1 value */
	capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
  
	/* Set the TIM4 Capture Compare1 Register value */
	__HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, (CCR1Val + capture));
}

 /**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	/* set tick to current debounce time*/
	DebounceTime = HAL_GetTick();

	/* subtract last debounce tick time with current*/
	DebounceTime=DebounceTime-LastDebounceTime;

	/* check, if EXTI0 was called, check if time since last call is bigger than 50ms*/
	if(GPIO_Pin == GPIO_PIN_0 && DebounceTime>50)
	{

		/* Test on the command: Recording */
		if (CmdIndex == CMD_RECORD)
		{
			/* Switch to stop command */
			CmdIndex = CMD_STOP;
		}

		else
		{
			/* Default Command Index: Record command */
			CmdIndex = CMD_RECORD;
		}

		/* set tick to debounce time -> will be compared in beginning of next EXTI0 callback*/
		LastDebounceTime = HAL_GetTick();
	}
} 

