#ifndef I2C_COMM_CONTROLLER_H_
#define I2C_COMM_CONTROLLER_H_

#include "ti_msp_dl_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FRAME STRUCTURE */

/* Control Byte */
/*! @brief Control index */
#define CTRL_IDX                     (0)
/*! @brief Control size */
#define CTRL_SIZE                    (1)
/*! @brief Control mask */
#define CMD_MASK                     (0x80)
/*! @brief CRC mask */
#define CRC_MASK                     (0x40)
/*! @brief Length mask */
#define LEN_MASK                     (0x3F)
/*! @brief Write command */
#define WRITE_CMD                    (0x80)
/*! @brief Read command */
#define READ_CMD                     (0x00)
/*! @brief Error Mask */
#define ERROR_MASK                   (0x80)  

/* Address  */
/*! @brief Address index */
#define ADDR_IDX                    (CTRL_IDX + CTRL_SIZE)
/*! @brief Address size */
#define ADDR_SIZE                   (4)
/*! @brief Address range start */
#define ADDR_RANGE_START            (0x20100000)
/*! @brief Address range end */
#define ADDR_RANGE_END              (0x20307FFF)

/* Data */
/*! @brief Command Data index */
#define DATA_IDX                    (ADDR_IDX + ADDR_SIZE)
/*! @brief Response Data index */
#define RESP_DATA_IDX               (CTRL_IDX + CTRL_SIZE)
/*! @brief Maximum data size */
#define MAX_DATA_SIZE               (64)

/* OFFSET is from the end of frame */
/* CRC */
/*! @brief CRC offset */
#define CRC_OFFSET                  (0)
/*! @brief CRC size */
#define CRC_SIZE                    (2)

/*! @brief Maximum buffer size */
#define MAX_BUFFER_SIZE             (CTRL_SIZE + ADDR_SIZE + MAX_DATA_SIZE + CRC_SIZE)
/*! @brief Maximum response size */
#define MAX_RESP_SIZE               (CTRL_SIZE + MAX_DATA_SIZE + CRC_SIZE)

/*! I2C Target Addr configured in the example */
/*! @brief Default Target Address */
#define DEF_TARGET_ADDR             (0x48)

/*! @brief Buffer Info structure */
typedef struct
{
    /*! Buffer array */
    _Alignas(uint32_t) uint8_t buffer[MAX_BUFFER_SIZE];
    /*! Pointer */
    uint8_t ptr;
    /*! Length */
    uint8_t len;
} BufferInfo;

/*! @brief Frame Info structure*/
typedef struct
{
    /*! CRC */
    uint16_t crc;
    /*! Control Byte */
    uint8_t ctrl;
    /*! Data */
    uint8_t data[MAX_DATA_SIZE];
} FrameInfo;

/*! @brief I2C status */
typedef enum
{
    /*! I2C IDLE state */
    I2C_STATUS_IDLE = 0,
    /*! I2C Tx Started state */
    I2C_STATUS_TX_STARTED,
    /*! I2C Tx in Progress state */
    I2C_STATUS_TX_INPROGRESS,
    /*! I2C Tx Complete state */
    I2C_STATUS_TX_COMPLETE,
    /*! I2C Rx Started state */
    I2C_STATUS_RX_STARTED,
    /*! I2C Rx in Progress state */
    I2C_STATUS_RX_INPROGRESS,
    /*! I2C Rx Complete state */
    I2C_STATUS_RX_COMPLETE,
    /*! I2C Error state */
    I2C_STATUS_ERROR,
} I2C_Status;

typedef enum
{
    /*! No error */
    ERROR_TYPE_NONE         = 0x00,
    /*! Mismatch CRC error */
    ERROR_TYPE_MISMATCH_CRC = 0xE1,
    /*! Error in Address range */
    ERROR_TYPE_ADDR_RANGE   = 0xE2,
} ErrorType;

/*! @brief I2C instance */
typedef struct
{
    /*! Transmit message */
    BufferInfo txMsg;
    /*! Receive message */
    BufferInfo rxMsg;

    /*! Data length */
    uint8_t dataLen;
    /*! CRC enabled */
    _Bool isCrc;

    /*! I2C status */
    I2C_Status status;
    /*! I2C error type */
    ErrorType error;
} I2C_Instance;

/*! @brief I2C Command types */
typedef enum 
{
    /*! I2C Read Command */
    READ_COMMAND = 0x00,
    /*! I2C Write Command */
    WRITE_COMMAND = 0x80,
} CommandType;

/*! @brief I2C Command Info */
typedef struct
{
    /*! Target Address */
    uint32_t targetAddr;
    /*! I2C Command Type */
    CommandType commandType;
    /*! Address */
    uint32_t addr;
    /*! Pointer to array of Data */
    uint8_t* dataArray;
    /*! Data Size */
    uint8_t dataSize;
    /*! CRC Enable*/
    bool crcEnable;
} I2C_CommandInfo;

/*! @brief I2C Response Info */
typedef struct
{
    /*! Response Frame Info */
    FrameInfo frame;
    /*! Response data size */
    uint8_t dataSize;
    /*! Response error status */
    ErrorType status;
    /*! Response received */
    bool received; 
} I2C_ResponseInfo;

/**
 * @brief     Initializes I2C_Instance Handle
 * @param[in] I2C_handle    Pointer to I2C_Instance
 */
void I2C_init(I2C_Instance *I2C_handle);

/**
 * @brief     Prepares I2C Command frame and stores in TX Buffer to transmit
 * @param[in] I2C_handle     Pointer to I2C_Instance
 * @param[in] command        Pointer to I2C_CommandInfo
 */
void I2C_sendCommand(I2C_Instance *I2C_handle,I2C_CommandInfo *command);

/**
 * @brief     Sends I2C Read command to get the response from target
 * @param[in] I2C_handle     Pointer to I2C_Instance
 * @param[in] targetAddr     I2C Target Address
 */
void I2C_getResponse(I2C_Instance* I2C_handle,uint32_t targetAddr);

/**
 * @brief     Decodes the received data in Rx Buffer for response
 * @param[in] I2C_handle     Pointer to I2C_Instance
 * @param[in] response       Pointer to I2C_ResponseInfo
 */
void I2C_decodeResponse(I2C_Instance *I2C_handle,I2C_ResponseInfo *response);

#ifdef __cplusplus
}
#endif

#endif /* I2C_COMM_CONTROLLER_H_ */
