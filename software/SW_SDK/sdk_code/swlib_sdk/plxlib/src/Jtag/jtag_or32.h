#ifndef _JTAG_OR32_H_
#define _JTAG_OR32_H_

#define INSTRUCTION_IDCODE 0x2
#define INSTRUCTION_CHAIN_SELECT 0x3
#define INSTRUCTION_DEBUG 0x8
#define INSTRUCTION_BYPASS 0xf

#define ATPG_CLOCK_CHAIN 0x2
//#define ATPG_SCAN_CHAIN 0x3
#define ATPG_SCAN_CHAIN 0x6
#define REGISTER_SCAN_CHAIN 0x4
#define WISHBONE_SCAN_CHAIN 0x5
#define TRACE_SCAN_CHAIN 0x3
typedef struct _MPT_SDK_STA
{
	U32 bstop;
	U32 berr;
	U32 err_code;      
	U32 status_code;
	U32 status_set[32];
}MPT_SDK_STA;

typedef struct _MPT_DEVICE
{
	U32 location_id;			/*USB LocationID to indicate which port the MPTTool connected*/
	U32 adapter_handle;			/*use for SDK												 */
	U32 device_exist_flag;		/*device exist flag											 */
	MPT_SDK_STA status;			/*device status												 */
	U32 event_status;			/*device sync event											 */
}MPT_DEVICE;

typedef struct _ST_WISHBONE_SCAN_CHAIN  //without CRC_IN and the redundant bit
{
	U32 LTXREGA;			
	U32 RCRBH_CYC;			
	U32 LTMMCFG;				
	U32 LTWD_L;	
	U32 LTWD_H;					
	U32 BE;	
	U32 MIO;			
	U32 RW;	
	U32 ADDR_L;					
	U32 ADDR_H;				
}ST_WISHBONE_SCAN_CHAIN;

enum {
	_UP_FTC_SUCCESS = 0, // FTC_OK
	_UP_FTC_INVALID_HANDLE = 1001, // FTC_INVALID_HANDLE
	_UP_FTC_DEVICE_NOT_FOUND = 1002, //FTC_DEVICE_NOT_FOUND
	_UP_FTC_DEVICE_NOT_OPENED = 1003, //FTC_DEVICE_NOT_OPENED
	_UP_FTC_IO_ERROR = 1004, //FTC_IO_ERROR
	_UP_FTC_INSUFFICIENT_RESOURCES = 1005, // FTC_INSUFFICIENT_RESOURCES
	_UP_FTC_CRC_ERROR = 1006,//FTC_CRC_ERROR
};
 U32  orjtag_init(void);
 U32  orjtag_device_enumerate(MPT_DEVICE *mpt_device, U32 *number_of_adapter);
 U32  orjtag_device_close(MPT_DEVICE *mpt_device);
 U32  orjtag_close(MPT_DEVICE *mpt_device);
 U32  orjtag_wishbone_write(MPT_DEVICE *mpt_device, U32 address, U32 length, const U8 *buffer, BOOL convertflag);
 U32  orjtag_wishbone_read(MPT_DEVICE *mpt_device, U32 address, U32 length, U8 *buffer, BOOL convertflag);
 U32  orjtag_debug_read(MPT_DEVICE *mpt_device, U32 *reg_value);
 U32  orjtag_debug_write(MPT_DEVICE *mpt_device, U32 reg_value);
 U32  orjtag_reg_read(MPT_DEVICE *mpt_device, U32 address, U32 *reg_value);
 U32  orjtag_reg_write(MPT_DEVICE *mpt_device, U32 address, U32 reg_value);
 U32  orjtag_idcode(MPT_DEVICE *mpt_device, U32 *idcode);

 U32  orjtag_atpgclock_write(MPT_DEVICE *mpt_device, U32 bitNum, U32 ByteNum, const U8 *buffer);
 U32  orjtag_atpgscan_write_vt3455(MPT_DEVICE *mpt_device, U32 bitNum, U32 ByteNum, const U8 *buffer);
// U32  orjtag_atpgscan_write_vt3456(MPT_DEVICE *mpt_device, U32 bitNum, U32 ByteNum, const U8 *buffer);
 U32  orjtag_registerscan_write(MPT_DEVICE *mpt_device,  U32 address, U32 reg_value, U32 RW);
 U32  orjtag_wishbonescan_write(MPT_DEVICE *mpt_device, ST_WISHBONE_SCAN_CHAIN* p_wishbone_scan_chain);
U32 jtag_set_chain(unsigned int index, U32 chain);

U32  orjtag_tracescan_read(MPT_DEVICE *mpt_device);
U32   orjtag_tracescan_write(MPT_DEVICE *mpt_device,U32 TSEL,U32 QSEL,U32 SSEL,U32 RECSEL,U32 MODER);
#endif /* _JTAG_OR32_H_ */
