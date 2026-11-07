//#include <string.h>
#include "bsp_usart.h"
#include "stdarg.h"
#include "main.h"
#include "bsp_log.h"

void DebugUart_SetEnabled(int enabled) {
    GPIO_InitTypeDef gpio={0};
    gpio.Pin=DAP_UART_RX_Pin|DAP_UART_TX_Pin;
    gpio.Pull=GPIO_NOPULL;
    gpio.Speed=GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Mode=enabled?GPIO_MODE_AF_PP:GPIO_MODE_INPUT;
    gpio.Alternate=GPIO_AF7_USART1;
    /* Disconnect the pins rather than stopping the UART clock: generated
     * printf retargeting can still finish internally without driving the bus. */
    HAL_GPIO_Init(DAP_UART_RX_GPIO_Port,&gpio);
}
//
//
//_Bool Uart_Error_Flag = 0;
//extern DMA_HandleTypeDef hdma_usart1_rx;
//extern DMA_HandleTypeDef hdma_usart1_tx;
char RxBuffer1[RXBUFFERSIZE];   //��������
//__IO uint16_t Rec_Length1 = 0;
//__IO _Bool Rec_Complete1 = 0;
//
//
/////�ض���c�⺯��printf�����ڣ��ض�����ʹ��printf����
//int fputc(int ch, FILE *f) {
//    HAL_UART_Transmit(&huart1, (uint8_t *) &ch, 1, 0xffff);
//    while (!(USART1->SR & UART_FLAG_TXE));
//    return (ch);
//}
//
/////�ض���c�⺯��scanf�����ڣ���д����ʹ��scanf��getchar�Ⱥ���
//int fgetc(FILE *f) {
//    /* �ȴ������������� */
//    uint8_t temp;
//    HAL_UART_Receive(&huart1, &temp, 1, 0xffff);
//    return temp;
//}
//
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//    if (huart == &huart1) { ;
//    }
//}
//
///**************************************************************************
// * @brief  ���ڴ���ص�����
// * @retval
//**************************************************************************/
//void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
//    if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_PE)) != RESET) {
//        // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)"00000", 5);
//        __HAL_UNLOCK(huart);
//        __HAL_UART_CLEAR_PEFLAG(huart);
//    }
//    if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_FE)) != RESET) {
//        // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)"00001", 5);
//        __HAL_UNLOCK(huart);
//        __HAL_UART_CLEAR_FEFLAG(huart);
//    }
//
//    if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_NE)) != RESET) {
//        // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)"00002", 5);
//        __HAL_UNLOCK(huart);
//        __HAL_UART_CLEAR_NEFLAG(huart);
//    }
//
//    if ((__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) != RESET) {
//        // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)"00003", 5);
//        __HAL_UNLOCK(huart);
//        __HAL_UART_CLEAR_OREFLAG(huart);
//    }
//
//    // �����־λ��
//    Uart_Error_Flag = 1;
//}
//
int Printf(char *formatString, ...) {
    char TXBuffer[TX_DATA_SIZE];
    int length;
    va_list args;
    va_start(args, formatString);
    length = vsnprintf(TXBuffer, TX_DATA_SIZE, formatString, args);
    va_end(args);
    if(length>0) {
        size_t stored=(size_t)length;
        if(stored>=sizeof(TXBuffer)) stored=sizeof(TXBuffer)-1;
        DebugLog_Write(TXBuffer,stored);
    }
    return length;
}
//
//uint16_t DMA_Printf(char *formatString, ...) {
//    char TXBuffer[TX_DATA_SIZE];
//    uint16_t length;
//    va_list args;
//    va_start(args, formatString);
//    length = vsnprintf(TXBuffer, TX_DATA_SIZE, formatString, args);
//    while (HAL_DMA_GetState(&hdma_usart1_tx) == HAL_DMA_STATE_BUSY);
//    if (huart1.gState != HAL_UART_STATE_READY)HAL_UART_Abort(&huart1);
//    __HAL_DMA_DISABLE(&hdma_usart1_tx);
//    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *) TXBuffer, length) != HAL_OK) //�ж��Ƿ�����������������쳣������쳣�жϺ���
//    {
//        //return;
//    }
//    while (__HAL_DMA_GET_COUNTER(&hdma_usart1_tx) != 0);//�ȷ����������
//    va_end(args);
//    return length;
//}
//
//void USART_DMARec_Stop(UART_HandleTypeDef *huart) {
//    uint32_t dmarequest = 0x00U;
//    /* Stop UART DMA Rx request if ongoing */
//    dmarequest = HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR);
//    if ((huart->RxState == HAL_UART_STATE_BUSY_RX) && dmarequest) {
//        CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);
//
//        /* Abort the UART DMA Rx channel */
//        if (huart->hdmarx != NULL) {
//            HAL_DMA_Abort(huart->hdmarx);
//        }
//        /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
//        CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
//        CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);
//
//        /* In case of reception waiting for IDLE event, disable also the IDLE IE interrupt source */
//        if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE) {
//            CLEAR_BIT(huart->Instance->CR1, USART_CR1_IDLEIE);
//        }
//
//        /* At end of Rx process, restore huart->RxState to Ready */
//        huart->RxState = HAL_UART_STATE_READY;
//        huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;
//    }
//}
//
//#define USART_IDLE_FLAG    (hdma_usart1_tx.State!=HAL_DMA_STATE_BUSY && hdma_usart2_tx.State!=HAL_DMA_STATE_BUSY)
//#define USART_IDLE_OK 1
//
void USART1_IDLE_Handler(void) {
//    uint16_t temp;
//    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET) {
//        if (hdma_usart1_tx.State != HAL_DMA_STATE_BUSY && Rec_Complete1 == 0)
//            //���DMA�����Ƿ���ɣ��������Ļ������������������ж�̫Ƶ�����ᵼ��DMA��ǰ�������·�����ֹ��ס
//        {
//            USART_DMARec_Stop(&huart1);
//            temp = __HAL_DMA_GET_COUNTER(
//                    &hdma_usart1_rx);// ��ȡDMA��δ��������ݸ���   //temp  = hdma_usart1_rx.Instance->NDTR;//��ȡNDTR�Ĵ��� ��ȡDMA��δ��������ݸ�����
//            Rec_Length1 = RXBUFFERSIZE - temp; //�ܼ�����ȥδ��������ݸ������õ��Ѿ����յ����ݸ���
//            Rec_Complete1 = 1;    // ������ɱ�־λ��1
//        }
//        __HAL_UART_CLEAR_IDLEFLAG(&huart1);//�����־λҪ�ų���������Ȼ�޷������־λ����һֱ���ж�handler��ѭ��
//    }
}
//
//
//static struct UartHandler_DriverStruct *
//USART_HandlerMount(UartHandle_FuncTypedef handler_func, UartHandle_FinishCallBackFuncTypedef callback_func,
//                   void *handler_userdata, void *callback_userdata) {
//    if (handler_func == NULL)
//        return &HandlerManager;
//    while (!HandlerManager.is_idle);
//    HandlerManager.handler_function = handler_func;
//    HandlerManager.callback_func = callback_func;
//    HandlerManager.handler_userdata = handler_userdata;
//    HandlerManager.callback_userdata = callback_userdata;
//    return &HandlerManager;
//}
//
//static void USART_HandlerRun(uint16_t timeout_ms, uint8_t need_reset) {
//    if (HandlerManager.handler_function == NULL)return;
//    HandlerManager.timeout_ms = timeout_ms;
//    HandlerManager.need_reset = need_reset;
//    HandlerManager.is_idle = 0;
//}
//
//static void USART_ManagerReset(void) {
//    HandlerManager.is_idle = 1;
//    HandlerManager.handler_function = NULL;
//    HandlerManager.callback_func = NULL;
//    HandlerManager.handler_userdata = NULL;
//    HandlerManager.callback_userdata = NULL;
//    HandlerManager.timeout_ms = 1000;
//    HandlerManager.need_reset = 1;
//}
//
UartHandler_DriverTypedef HandlerManager = {
        .handler_function = NULL,
        .callback_func = NULL,
//        .mount = USART_HandlerMount,
//        .run = USART_HandlerRun,
//        .reset = USART_ManagerReset,
        .handler_userdata = NULL,
        .callback_userdata = NULL,
        .timeout_ms = 1000,
        .is_idle = 1,
        .need_reset = 1,
};
//
void USART1_REC_Handler(void) {
//    static uint32_t time_begin = 0;
//    static uint8_t flag = 0;
//    if (Rec_Complete1 == 1)  //������ɱ�־
//    {
//        __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);
//        //======USART1_LOGIC_BEGIN======
////        if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *) RxBuffer1, Rec_Length1) != HAL_OK) //�ж��Ƿ�����������������쳣������쳣�жϺ���
////        {
////            //return;
////        }
////        while (__HAL_DMA_GET_COUNTER(&hdma_usart1_tx) != 0);//�ȷ����������
//        if (!HandlerManager.is_idle && HandlerManager.handler_function != NULL) {
//            HandlerManager.handler_function(RxBuffer1, Rec_Length1, HandlerManager.handler_userdata);
//            HandlerManager.is_idle = 1;//successful -> set is_idel to 1 !
//        }
//        //======USART1_LOGIC_END======
//        memset(RxBuffer1, 0, Rec_Length1);
//        Rec_Length1 = 0;//�������
//        Rec_Complete1 = 0;//������ս�����־λ
//        HAL_UART_Receive_DMA(&huart1, (uint8_t *) RxBuffer1, RXBUFFERSIZE);//���´�DMA����
//        __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
//    }
//    switch (flag) {
//        case 0: {
//            if (!HandlerManager.is_idle) {
//                time_begin = HAL_GetTick();
//                flag = 1;
//            }
//            break;
//        }
//        case 1: {
//            if (HandlerManager.is_idle || HAL_GetTick() - time_begin >= HandlerManager.timeout_ms)
//                flag = 2;
//            break;
//        }
//        case 2: {
//            if (HandlerManager.callback_func) {
//                if (HandlerManager.is_idle) {
//                    HandlerManager.callback_func(HAL_OK, HandlerManager.callback_userdata);
//                } else {
//                    HandlerManager.callback_func(HAL_TIMEOUT, HandlerManager.callback_userdata);
//                }
//                if (HandlerManager.need_reset)HandlerManager.reset();
//            }
//            flag = 0;
//            break;
//        }
//        default:
//            flag = 0;
//            break;
//    }
}
