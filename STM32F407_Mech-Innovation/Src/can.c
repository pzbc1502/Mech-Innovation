/**
  ******************************************************************************
  * File Name          : CAN.c
  * Description        : This file provides code for the configuration
  *                      of the CAN instances.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN 0 */

__IO CAN_t can = {0};

/* USER CODE END 0 */

CAN_HandleTypeDef hcan2;

/* CAN2 init function */
void MX_CAN2_Init(void)
{
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 14;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_4TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = ENABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = DISABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if(canHandle->Instance==CAN2)
  {
    /*
     * CAN2 is a slave CAN peripheral on STM32F4, so CAN1 clock must also be
     * enabled even though the bus is physically on CAN2 PB12/PB13.
     */
    __HAL_RCC_CAN1_CLK_ENABLE();
    __HAL_RCC_CAN2_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* CAN2 interrupt Init */
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{
  if(canHandle->Instance==CAN2)
  {
    __HAL_RCC_CAN2_CLK_DISABLE();
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN2 GPIO Configuration
    PB12     ------> CAN2_RX
    PB13     ------> CAN2_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);

    /* CAN2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN2_RX0_IRQn);
  }
}

/* USER CODE BEGIN 1 */

void USER_CAN2_Filter_Init(void)
{
  CAN_FilterTypeDef sFilterConfig;

  __IO uint8_t id_o, im_o;
  __IO uint16_t id_l, id_h, im_l, im_h;

  id_o = 0x00;
  id_h = (uint16_t)((uint16_t)id_o >> 5);
  id_l = (uint16_t)((uint16_t)id_o << 11) | CAN_ID_EXT;
  im_o = 0x00;
  im_h = (uint16_t)((uint16_t)im_o >> 5);
  im_l = (uint16_t)((uint16_t)im_o << 11) | CAN_ID_EXT;

  sFilterConfig.FilterBank = 14;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  sFilterConfig.FilterIdHigh = id_h;
  sFilterConfig.FilterIdLow = id_l;
  sFilterConfig.FilterMaskIdHigh = im_h;
  sFilterConfig.FilterMaskIdLow = im_l;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 14;

  if(HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

void can_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
  static uint32_t TxMailbox;
  __IO uint8_t i = 0, j = 0, k = 0, l = 0, packNum = 0, retry = 0;

  if((cmd == NULL) || (len < 3))
  {
    return;
  }

  j = len - 2;

  while(i < j)
  {
    k = j - i;

    can.CAN_TxMsg.StdId = 0x00;
    can.CAN_TxMsg.ExtId = ((uint32_t)cmd[0] << 8) | (uint32_t)packNum;
    can.txData[0] = cmd[1];
    can.CAN_TxMsg.IDE = CAN_ID_EXT;
    can.CAN_TxMsg.RTR = CAN_RTR_DATA;

    if(k < 8)
    {
      for(l = 0; l < k; l++, i++)
      {
        can.txData[l + 1] = cmd[i + 2];
      }
      can.CAN_TxMsg.DLC = k + 1;
    }
    else
    {
      for(l = 0; l < 7; l++, i++)
      {
        can.txData[l + 1] = cmd[i + 2];
      }
      can.CAN_TxMsg.DLC = 8;
    }

    retry = 0;
    while(HAL_CAN_AddTxMessage(&hcan2, (CAN_TxHeaderTypeDef *)(&can.CAN_TxMsg), (uint8_t *)(&can.txData), &TxMailbox) != HAL_OK)
    {
      if(++retry >= 50)
      {
        return;
      }
    }

    ++packNum;
  }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  if(hcan->Instance != CAN2)
  {
    return;
  }

  while(HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0U)
  {
    if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, (CAN_RxHeaderTypeDef *)(&can.CAN_RxMsg), (uint8_t *)(&can.rxData)) == HAL_OK)
    {
      can.rxFrameFlag = true;
    }
    else
    {
      break;
    }
  }
}

/* USER CODE END 1 */
