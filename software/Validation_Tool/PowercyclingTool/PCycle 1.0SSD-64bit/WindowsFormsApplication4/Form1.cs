using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using System.IO;
using System.IO.Compression;
using System.Diagnostics;
using System.Net;
namespace WindowsFormsApplication4
{
    public partial class Form1 : Form
    {
        private const string App_Title = "FT2232/FT4232 SPI Device C# .NET Test Application";

        private const string Dll_Version_Label = "FT2232/FT4232 SPI DLL Version = ";

        private const string Device_Name_Label = "Device Name = ";

        private string Disk_Device = "";
        private bool Disk_Test = false;
        private bool Disk_File_D = false;
        private uint Disk_Cycle = 1;
        private uint Disk_Fail = 0;
        private uint Disk_WriteTimes = 0;
        private uint Disk_FileWriteCycle = 0;
        private uint Disk_FileName = 0;
        private uint Disk_FileWriteTimes = 0;
        private int Disk_PowerOffTime = 0;
        private int Disk_PowerOnTime = 0;
        private int Disk_PowerOffTimeRandom = 0;
        private string Disk_SourceFile = "";
        private string Disk_Exception = "";
        string year = "";
        string week = "";
        string day = "";

        Stopwatch sw = new Stopwatch();//Stopwatch類別在System.Diagnostics命名空間裡

        private enum HI_SPEED_DEVICE_TYPES// : uint
        {
            FT2232H_DEVICE_TYPE = 1,
            FT4232H_DEVICE_TYPE = 2
        };
        //====================================USB
        //[StructLayout(LayoutKind.Sequential)]
        public struct DEV_BROADCAST_VOLUME
        {
            public int dbcv_size;
            public int dbcv_devicetype;
            public int dbcv_reserved;
            public int dbcv_unitmask;
        }
        //====================================USB

        WindowsFormsApplication4.CTestBase pRef = null;
        WindowsFormsApplication4.CFileItem pFileItem;
        WindowsFormsApplication4.CSetting pSettingItem;
       
        WindowsFormsApplication4.CCMPSeniorSetting pCMPSeniorSetting;
        WindowsFormsApplication4.CCMPSetting pCMPSetting;
       
        private const uint FTC_SUCCESS = 0;
        private const uint FTC_DEVICE_IN_USE = 27;

        private const uint MAX_NUM_DEVICE_NAME_CHARS = 100;
        private const uint MAX_NUM_CHANNEL_CHARS = 5;

        private const uint MAX_NUM_DLL_VERSION_CHARS = 10;
        private const uint MAX_NUM_ERROR_MESSAGE_CHARS = 100;

        // To communicate with the 93LC56(2048 word) EEPROM, the maximum frequency the clock can be set is 1MHz 
        private const uint MAX_FREQ_93LC56_CLOCK_DIVISOR = 29;    // equivalent to 1MHz

        private const bool ADBUS3ChipSelect = false;

        private const uint ADBUS2DataIn = 0;

        private const int WRITE_CONTROL_BUFFER_SIZE = 256;
        private const int WRITE_DATA_BUFFER_SIZE = 65536;
        private const int READ_DATA_BUFFER_SIZE = 65536;
        private const int READ_CMDS_DATA_BUFFER_SIZE = 131071;

        private const uint SPIEWENCmdIndex = 0;
        private const uint SPIEWDSCmdIndex = 1;
        private const uint SPIERALCmdIndex = 2;

        private const uint MAX_SPI_93LC56_CHIP_SIZE_IN_WORDS = 128;

        private const uint NUM_93LC56B_CMD_CONTOL_BITS = 11;
        private const uint NUM_93LC56B_CMD_CONTOL_BYTES = 2;

        private const uint NUM_93LC56B_CMD_DATA_BITS = 16;
        private const uint NUM_93LC56B_CMD_DATA_BYTES = 2;


        private FTC_CHIP_SELECT_PINS ChipSelectsDisableStatesA;
        private FTH_INPUT_OUTPUT_PINS HighInputOutputPinsA;
        private IntPtr ftHandleA = IntPtr.Zero;

        //**************************************************************************
        //
        // TYPE DEFINITIONS
        //
        //**************************************************************************

        public struct FTC_CHIP_SELECT_PINS
        {
            public bool bADBUS3ChipSelectPinState;
            public bool bADBUS4GPIOL1PinState;
            public bool bADBUS5GPIOL2PinState;
            public bool bADBUS6GPIOL3PinState;
            public bool bADBUS7GPIOL4PinState;
        }

        public struct FTC_INPUT_OUTPUT_PINS
        {
            public bool bPin1InputOutputState;
            public bool bPin1LowHighState;
            public bool bPin2InputOutputState;
            public bool bPin2LowHighState;
            public bool bPin3InputOutputState;
            public bool bPin3LowHighState;
            public bool bPin4InputOutputState;
            public bool bPin4LowHighState;
        }

        public struct FTH_INPUT_OUTPUT_PINS
        {
            public bool bPin1InputOutputState;
            public bool bPin1LowHighState;
            public bool bPin2InputOutputState;
            public bool bPin2LowHighState;
            public bool bPin3InputOutputState;
            public bool bPin3LowHighState;
            public bool bPin4InputOutputState;
            public bool bPin4LowHighState;
            public bool bPin5InputOutputState;
            public bool bPin5LowHighState;
            public bool bPin6InputOutputState;
            public bool bPin6LowHighState;
            public bool bPin7InputOutputState;
            public bool bPin7LowHighState;
            public bool bPin8InputOutputState;
            public bool bPin8LowHighState;
        }

        public struct FTC_LOW_HIGH_PINS
        {
            public bool bPin1LowHighState;
            public bool bPin2LowHighState;
            public bool bPin3LowHighState;
            public bool bPin4LowHighState;
        }

        public struct FTH_LOW_HIGH_PINS
        {
            public bool bPin1LowHighState;
            public bool bPin2LowHighState;
            public bool bPin3LowHighState;
            public bool bPin4LowHighState;
            public bool bPin5LowHighState;
            public bool bPin6LowHighState;
            public bool bPin7LowHighState;
            public bool bPin8LowHighState;
        }

        public struct FTC_INIT_CONDITION
        {
            public bool bClockPinState;
            public bool bDataOutPinState;
            public bool bChipSelectPinState;
            public bool ChipSelectPin;
        }

        public struct FTC_WAIT_DATA_WRITE
        {
            public bool bWaitDataWriteComplete;
            public uint WaitDataWritePin;
            public bool bDataWriteCompleteState;
            public uint DataWriteTimeoutmSecs;
        }

        public struct FTC_HIGHER_OUTPUT_PINS
        {
            public bool bPin1State;
            public bool bPin1ActiveState;
            public bool bPin2State;
            public bool bPin2ActiveState;
            public bool bPin3State;
            public bool bPin3ActiveState;
            public bool bPin4State;
            public bool bPin4ActiveState;
        }

        public struct FTH_HIGHER_OUTPUT_PINS
        {
            public bool bPin1State;
            public bool bPin1ActiveState;
            public bool bPin2State;
            public bool bPin2ActiveState;
            public bool bPin3State;
            public bool bPin3ActiveState;
            public bool bPin4State;
            public bool bPin4ActiveState;
            public bool bPin5State;
            public bool bPin5ActiveState;
            public bool bPin6State;
            public bool bPin6ActiveState;
            public bool bPin7State;
            public bool bPin7ActiveState;
            public bool bPin8State;
            public bool bPin8ActiveState;
        }

        public struct FTC_CLOSE_FINAL_STATE_PINS
        {
            public bool bTCKPinState;
            public bool bTCKPinActiveState;
            public bool bTDIPinState;
            public bool bTDIPinActiveState;
            public bool bTMSPinState;
            public bool bTMSPinActiveState;
        }

        [DllImport("User32.dll", EntryPoint = "PostMessage")]
        public static extern IntPtr PostMessage(int hWnd, int Msg, IntPtr wParam, int lParam);
        //**************************************************************************
        //
        // FUNCTION IMPORTS FROM FTCI2C DLL
        //
        //**************************************************************************

        // Built-in Windows API functions to allow us to dynamically load our own DLL.
        [DllImportAttribute("FTCSPI64.dll", EntryPoint = "SPI_GetDllVersion", CallingConvention = CallingConvention.Cdecl)]
        static extern uint GetDllVersion(byte[] pDllVersion, uint buufferSize);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetErrorCodeString(string language, uint statusCode, byte[] pErrorMessage, uint bufferSize);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetNumHiSpeedDevices(ref uint NumHiSpeedDevices);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetHiSpeedDeviceNameLocIDChannel(uint deviceNameIndex, byte[] pDeviceName, uint deviceNameBufferSize, ref uint locationID, byte[] pChannel, uint channelBufferSize, ref uint hiSpeedDeviceType);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_OpenHiSpeedDevice(string DeviceName, uint locationID, string channel, ref IntPtr pftHandle);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetHiSpeedDeviceType(IntPtr ftHandle, ref uint hiSpeedDeviceType);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_Close(IntPtr ftHandle);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_CloseDevice(IntPtr ftHandle, ref FTC_CLOSE_FINAL_STATE_PINS pCloseFinalStatePinsData);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_InitDevice(IntPtr ftHandle, uint clockDivisor);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_TurnOnDivideByFiveClockingHiSpeedDevice(IntPtr ftHandle);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_TurnOffDivideByFiveClockingHiSpeedDevice(IntPtr ftHandle);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_SetDeviceLatencyTimer(IntPtr ftHandle, byte timerValue);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetDeviceLatencyTimer(IntPtr ftHandle, ref byte timerValue);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetHiSpeedDeviceClock(uint ClockDivisor, ref uint clockFrequencyHz);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetClock(uint clockDivisor, ref uint clockFrequencyHz);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_SetClock(IntPtr ftHandle, uint clockDivisor, ref uint clockFrequencyHz);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_SetLoopback(IntPtr ftHandle, bool bLoopBackState);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_SetHiSpeedDeviceGPIOs(IntPtr ftHandle, ref FTC_CHIP_SELECT_PINS pChipSelectsDisableStates, ref FTH_INPUT_OUTPUT_PINS pHighInputOutputPinsData);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_GetHiSpeedDeviceGPIOs(IntPtr ftHandle, out FTH_LOW_HIGH_PINS pHighPinsInputData);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_WriteHiSpeedDevice(IntPtr ftHandle, ref FTC_INIT_CONDITION pWriteStartCondition, bool bClockOutDataBitsMSBFirst, bool bClockOutDataBitsPosEdge, uint numControlBitsToWrite, byte[] pWriteControlBuffer, uint numControlBytesToWrite, bool bWriteDataBits, uint numDataBitsToWrite, byte[] pWriteDataBuffer, uint numDataBytesToWrite, ref FTC_WAIT_DATA_WRITE pWaitDataWriteComplete, ref FTH_HIGHER_OUTPUT_PINS pHighPinsWriteActiveStates);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_ReadHiSpeedDevice(IntPtr ftHandle, ref FTC_INIT_CONDITION pReadStartCondition, bool bClockOutControBitsMSBFirst, bool bClockOutControBitsPosEdge, uint numControlBitsToWrite, byte[] pWriteControlBuffer, uint numControlBytesToWrite, bool bClockInDataBitsMSBFirst, bool bClockInDataBitsPosEdge, uint numDataBitsToRead, byte[] pReadDataBuffer, out uint pnumDataBytesReturned, ref FTH_HIGHER_OUTPUT_PINS pHighPinsReadActiveStates);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_ClearDeviceCmdSequence(IntPtr ftHandle);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_AddHiSpeedDeviceReadCmd(IntPtr ftHandle, ref FTC_INIT_CONDITION pReadStartCondition, bool bClockOutControBitsMSBFirst, bool bClockOutControBitsPosEdge, uint numControlBitsToWrite, byte[] pWriteControlBuffer, uint numControlBytesToWrite, bool bClockInDataBitsMSBFirst, bool bClockInDataBitsPosEdge, uint numDataBitsToRead, ref FTH_HIGHER_OUTPUT_PINS pHighPinsReadActiveStates);

        [DllImportAttribute("FTCSPI64.dll", CallingConvention = CallingConvention.Cdecl)]
        static extern uint SPI_ExecuteDeviceCmdSequence(IntPtr ftHandle, byte[] pReadCmdSequenceDataBuffer, out uint pnumDataBytesReturned);
        //------------------------------
        public Thread thread1;//複製用

        public byte[] BuffderBytes = null;
        public int BufferSize;
        public BufferedStream bs = null;
        public BufferedStream bsout = null;

        //------------------------------
        private TextWriter output;
        public Form1()
        {
            InitializeComponent();
            this.output = TextWriter.Synchronized(new TextBoxWriter(this.textBox8));
            this.ChannelComboBox.Items.Clear();
            this.ChannelComboBox.Items.Add("A");
            this.ChannelComboBox.Items.Add("B");
            this.ChannelComboBox.Text = "A";
            /*Bob qiu 2013-9-29*/
            this.TMethodCombox.Items.Clear();
            this.TMethodCombox.Items.Add("Simple Test");
            this.TMethodCombox.Items.Add("Medium Test");
            this.TMethodCombox.Items.Add("Senior Test");
            this.TMethodCombox.Text = "Simple Test";

            this.checkBox_enable_big.Checked = false;
            this.checkBox_enable_medium.Checked = true;
            this.checkBox_enable_small.Checked = true;
            this.com_value_big.Items.Add("0xAA");
            this.com_value_big.Items.Add("0x55");
            this.com_value_big.Items.Add("Random Value");
            this.com_value_big.Text = "0xAA";


            this.com_value_medium.Items.Add("0xAA");
            this.com_value_medium.Items.Add("0x55");
            this.com_value_medium.Items.Add("Random Value");
            this.com_value_medium.Text = "0xAA";

            this.com_value_small.Items.Add("0xAA");
            this.com_value_small.Items.Add("0x55");
            this.com_value_small.Items.Add("Random Value");
            this.com_value_small.Text = "0xAA";

            this.pFileItem = new CFileItem();
            this.pSettingItem = new CSetting();

            UpdateListHeader();
            /*Bob qiu 2013-9-29*/
            Open_FD2232H();
            pRef = null;
            this.pCMPSetting = new CCMPSetting();
            this.pCMPSeniorSetting = new CCMPSeniorSetting();
            //Form.CheckForIllegalCrossThreadCalls = false;//據說不安全，但我也不曉得哪裡不安全

        }
        public class TextBoxWriter : TextWriter
        {
            private TextBox _textbox;
            public TextBoxWriter(TextBox textbox)
            {
                this._textbox = textbox;
            }
            public override void Write(char value)
            {
                this.Write(new char[] { value }, 0, 1);
            }
            public override void Write(char[] buffer, int index, int count)
            {
                if (this._textbox.InvokeRequired)
                {
                    this._textbox.Invoke((Action<string>)this.AppendTextBox, new string(buffer, index, count));
                }
                else
                {
                    this.AppendTextBox(new string(buffer, index, count));
                }
            }
            private void AppendTextBox(string text)
            {
                this._textbox.AppendText(text);
            }
            public override Encoding Encoding
            {
                get { return Encoding.UTF8; }

            }
        }

        //====================================USB
        protected override void WndProc(ref Message m)
        {
            DEV_BROADCAST_VOLUME vol;

            const int WM_DEVICECHANGE = 0x219;
            const int DBT_DEVICEARRIVAL = 0x8000;
            const int DBT_DEVICEREMOVECOMPLETE = 0x8004;

            if (m.Msg == WM_DEVICECHANGE)
            {
                switch (m.WParam.ToInt32())
                {

                    case DBT_DEVICEARRIVAL:  //USB DISK 插入時觸發
                        if(textBox1.Text == "")
                        {
                            DeviceChange();//無設備預設值，有設備接上
                        }
                        //MessageBox.Show("DBT_DEVICEARRIVAL"); 
                        
                        vol = (DEV_BROADCAST_VOLUME)Marshal.PtrToStructure(m.LParam, typeof(DEV_BROADCAST_VOLUME));
                        //Console.WriteLine(FirstDriveFromMask(vol.dbcv_unitmask));
                        //Console.WriteLine(((FirstDriveFromMask(vol.dbcv_unitmask)).Length).ToString());
                        if (textBox1.Text == (FirstDriveFromMask(vol.dbcv_unitmask)).Substring(0,2))
                        {
                            Disk_Device = "CARD INSERT IN DISK ";
                            Console.WriteLine("第{0}次設備就緒，檔案寫入完成{2}次，有{1}次失敗", Disk_Cycle, Disk_Fail, Disk_FileWriteCycle);
                            if (pRef != null)
                                pRef._Disk_PowerOff(false);
                            set_LED_G();//設備就緒
                           // MessageBox.Show("CARD INSERT IN DISK " + FirstDriveFromMask(vol.dbcv_unitmask));
                        }
                        break;
                    case DBT_DEVICEREMOVECOMPLETE:  //USB DISK 拔出時觸發
                        string diskfound = DiskChack(textBox1.Text);
                        if (diskfound == "Nofound" && Disk_Device == "CARD INSERT IN DISK ")
                        {
                            Disk_Device = "DEVICEREMOVECOMPLETE";
                            Console.WriteLine("設備移除");
                            if (pRef != null)
                                pRef._Disk_PowerOff(true);
                            set_LED_R();//設備移除
                            //MessageBox.Show("DBT_DEVICEREMOVECOMPLETE  " + textBox1.Text);
                        }
                        //MessageBox.Show("DBT_DEVICEREMOVECOMPLETE");
                        break;
                }
            }
            base.WndProc(ref m);
        }

        //---------------------------------------------------------------------------
        private string FirstDriveFromMask(int unitmask)
        {
            byte[] r = new byte[] { 0, (byte)':', 0 };
            int i;
            for (i = 0; i < 26; ++i)
            {
                if ((unitmask & 0x1) != 0)
                    break;
                unitmask = unitmask >> 1;
            }
            r[0] = (byte)(i + 65);
            string rr = System.Text.Encoding.Default.GetString(r); //byte[] 轉 string
            return rr;
        }
        private void DeviceChange()
        {
            foreach (DriveInfo di in DriveInfo.GetDrives())   
            {   
              if (di.DriveType == DriveType.Removable)   
              {
                  textBox1.Text = di.Name.Substring(0, 2);
                  //MessageBox.Show("偵測到 " + di.Name + " 抽取式存放裝置");   
              }
              if (di.DriveType == DriveType.Fixed)
              {
                  textBox1.Text = di.Name.Substring(0, 2);
                  //listBox1.Items.Add("偵測到" + di.Name + "固定式跌機");
              }
            }   
            
        }  
        //====================================USB
        //====================================System.Threading.Timer 執行緒計時器

        private class StateObjClass
        {
            // Used to hold parameters for calls to TimerTask.
            public int SomeValue;
            public System.Threading.Timer TimerReference;
            public bool TimerCanceled;
            public WindowsFormsApplication4.Form1 refForm1; 
        }

        public void RunTimer(int ThreadTime)
        {
            StateObjClass StateObj = new StateObjClass();
            StateObj.TimerCanceled = false;
            StateObj.refForm1 = this;
            StateObj.SomeValue = 1;
            System.Threading.TimerCallback TimerDelegate =
                new System.Threading.TimerCallback(TimerTask);

            // Create a timer that calls a procedure every 2 seconds.
            // Note: There is no Start method; the timer starts running as soon as 
            // the instance is created.
            System.Threading.Timer TimerItem =
                new System.Threading.Timer(TimerDelegate, StateObj, ThreadTime, ThreadTime);

            // Save a reference for Dispose.
            StateObj.TimerReference = TimerItem;

            // Run for ten loops.
            //while (StateObj.SomeValue < 2)
            //{
            //    // Wait one second.
            //    //System.Threading.Thread.Sleep(1000);
            //}


            // Request Dispose of the timer object.
            sw.Reset();
            sw = Stopwatch.StartNew();
            //System.Diagnostics.Debug.WriteLine("StateObj.TimerCanceled = true  Launched new thread  " + DateTime.Now.ToString());
            StateObj.TimerCanceled = true;
        }

        public void SendMessage()
        {
            Console.Write("abcdfd\n");
            Message msg = Message.Create(this.Handle, 0x219, IntPtr.Zero, IntPtr.Zero);
            Console.Write("abcdfd\n");
        }
        private void TimerTask(object StateObj)
        {
            StateObjClass State = (StateObjClass)StateObj;
            //string msg;
            const int WM_DEVICECHANGE = 0x219;
            const int DBT_DEVICEARRIVAL = 0x8000;
            const int DBT_DEVICEREMOVECOMPLETE = 0x8004;
            // Use the interlocked class to increment the counter variable.
            //System.Threading.Interlocked.Increment(ref State.SomeValue);
            //System.Diagnostics.Debug.WriteLine("Launched new thread  " + DateTime.Now.ToString());
            if (State.TimerCanceled)
            // Dispose Requested.
            {
                State.TimerReference.Dispose();//Stop the Timer thread
                //System.Diagnostics.Debug.WriteLine("Done  " + DateTime.Now.ToString());
                uint ftStatus = FTC_SUCCESS;

                HighInputOutputPinsA.bPin3InputOutputState = true;
                if (HighInputOutputPinsA.bPin3LowHighState)
                {
                    //WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " Power Down"+ "\r\n", "Log_" + year + week + day + ".txt");
                    Console.WriteLine("关闭设备:{0}", sw.ElapsedMilliseconds);
                    //this.label2.Text = "關閉設備時間：";
                    if(pRef != null)
                        pRef._Disk_PowerOff(true);
                    HighInputOutputPinsA.bPin3LowHighState = false;
                    //bob for testing 2013-10-15
                    //System.Threading.Thread.Sleep(100);
                    /*if (pRef != null)
                        pRef._Disk_PowerOff(false);
                    this.set_LED_G();*/

                    //State.refForm1.SendMessage();
                   // Message msg = Message.Create(State.refForm1.Handle, 0x219, IntPtr.Zero, IntPtr.Zero);
                   // PostMessage((int)State.refForm1.Handle, 0x219, new IntPtr(0x8000), 0);
                    
                }
                else
                {
                    //this.label2.Text = "啟動設備時間：";
                    //WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " Power Up" + "\r\n", "Log_" + year + week + day + ".txt");
                    Console.WriteLine("启动设备");
                    HighInputOutputPinsA.bPin3LowHighState = true;
                    if (pRef != null)
                        pRef._Disk_PowerOff(false);
                    //bob for testing 2013-10-15
                    //System.Threading.Thread.Sleep(100);
                    /*if (pRef != null)
                        pRef._Disk_PowerOff(true);
                    this.set_LED_R();*/
                    //Message msg = Message.Create(State.refForm1.Handle, 0x219, IntPtr.Zero, IntPtr.Zero);
                   // PostMessage((int)State.refForm1.Handle, 0x219, new IntPtr(0x8004), 0);
                    //State.refForm1.SendMessage();
                }
                ftStatus = SPI_SetHiSpeedDeviceGPIOs(ftHandleA, ref ChipSelectsDisableStatesA, ref HighInputOutputPinsA);
                if (HighInputOutputPinsA.bPin3LowHighState)
                {
                    WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " Power Up" + "\r\n", "Log_" + year + week + day + ".txt");
                }
                else
                {
                    WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " Power Down" + "\r\n", "Log_" + year + week + day + ".txt");
                }
            }
        }


        //====================================System.Threading.Timer 執行緒計時器


        private void Form1_Load(object sender, EventArgs e)
        {



        }


        private void Open_FD2232H()
        {
            FTC_CHIP_SELECT_PINS ChipSelectsDisableStates;
            FTH_INPUT_OUTPUT_PINS HighInputOutputPins;

            uint ftStatus = FTC_SUCCESS;
            uint numHiSpeedDevices = 0; // 32-bit unsigned integer
            uint hiSpeedDeviceIndex = 0;
            uint locationID = 0;
            uint hiSpeedDeviceType = 0;

            byte[] byteDllVersion = new byte[MAX_NUM_DLL_VERSION_CHARS];
            byte[] byteHiSpeedDeviceName = new byte[MAX_NUM_DEVICE_NAME_CHARS];
            byte[] byteHiSpeedDeviceChannel = new byte[MAX_NUM_CHANNEL_CHARS];

            byte timerValue = 0;

            bool bLoopBackState = false;

            string DllVersion;
            string hiSpeedChannel = null;
            string hiSpeedDeviceName = null;

            IntPtr ftHandle = IntPtr.Zero;

            this.ChannelComboBox.Items.Clear();// 通道列表 清除 我原本想在下面新增 但跟原本成是不一樣 *1


            ftStatus = GetDllVersion(byteDllVersion, MAX_NUM_DLL_VERSION_CHARS);
            DllVersion = Encoding.ASCII.GetString(byteDllVersion);
            // Trim strings to first occurrence of a null terminator character
            DllVersion = DllVersion.Substring(0, DllVersion.IndexOf("\0"));
            Console.WriteLine("DllVersion " + DllVersion);

            ftStatus = SPI_GetNumHiSpeedDevices(ref numHiSpeedDevices);
            Console.WriteLine("Devices num " + numHiSpeedDevices);

            if ((ftStatus == FTC_SUCCESS) && (numHiSpeedDevices > 0))
            {
                do
                {
                    ftStatus = SPI_GetHiSpeedDeviceNameLocIDChannel(hiSpeedDeviceIndex, byteHiSpeedDeviceName, MAX_NUM_DEVICE_NAME_CHARS, ref locationID, byteHiSpeedDeviceChannel, MAX_NUM_CHANNEL_CHARS, ref hiSpeedDeviceType);

                    if (ftStatus == FTC_SUCCESS)
                    {
                        hiSpeedChannel = Encoding.ASCII.GetString(byteHiSpeedDeviceChannel);
                        
                        // Trim strings to first occurrence of a null terminator character
                        hiSpeedChannel = hiSpeedChannel.Substring(0, hiSpeedChannel.IndexOf("\0"));

                        this.ChannelComboBox.Items.Add(hiSpeedChannel);// 通道列表 新增 但跟原本成是不一樣 *1

                        Console.WriteLine("hiSpeedChannel " + hiSpeedChannel);
                        Console.WriteLine("locationID " + locationID);
                        Console.WriteLine("Channel " + hiSpeedChannel);
                        Console.WriteLine("hiSpeedDeviceIndex " + hiSpeedDeviceIndex);

                    }
                    if (hiSpeedChannel == ChannelComboBox.Text)
                        break;
                    hiSpeedDeviceIndex = hiSpeedDeviceIndex + 1;
                }
                while ((ftStatus == FTC_SUCCESS) && (hiSpeedDeviceIndex < numHiSpeedDevices) && (hiSpeedChannel != null));
                //while ((ftStatus == FTC_SUCCESS) && (hiSpeedDeviceIndex < numHiSpeedDevices) && (hiSpeedChannel != null));
                //while ((ftStatus == FTC_SUCCESS) && (hiSpeedDeviceIndex < numHiSpeedDevices) && ((hiSpeedChannel != null) && (hiSpeedChannel != this.ChannelComboBox.Text))) ;

                //我取消掉了，好像沒啥用
                //if (ftStatus == FTC_SUCCESS)
                //{
                //    if ((hiSpeedChannel != null) && (hiSpeedChannel != this.ChannelComboBox.Text))
                //        ftStatus = FTC_DEVICE_IN_USE;// 不知道為啥要到 27
                //}

                

                if (ftStatus == FTC_SUCCESS)
                {
                    
                    hiSpeedDeviceName = Encoding.ASCII.GetString(byteHiSpeedDeviceName);
                    
                    // Trim strings to first occurrence of a null terminator character
                    hiSpeedDeviceName = hiSpeedDeviceName.Substring(0, hiSpeedDeviceName.IndexOf("\0"));
                   
                    // The ftHandle parameter is a pointer to a variable of type DWORD ie 32-bit unsigned integer

                   
                    ftStatus = SPI_OpenHiSpeedDevice(hiSpeedDeviceName, locationID, hiSpeedChannel, ref ftHandle);

                    if (ftStatus == FTC_SUCCESS)
                    {
                        this.DeviceNameLabel.Text = Device_Name_Label + hiSpeedDeviceName;
                        this.DeviceNameLabel.Refresh();

                        ftStatus = SPI_GetHiSpeedDeviceType(ftHandle, ref hiSpeedDeviceType);

                        if (ftStatus == FTC_SUCCESS)
                        {
                            if (hiSpeedDeviceType == (uint)HI_SPEED_DEVICE_TYPES.FT4232H_DEVICE_TYPE)
                                this.FT4232HRadioButton.Checked = true;
                            else if (hiSpeedDeviceType == (uint)HI_SPEED_DEVICE_TYPES.FT2232H_DEVICE_TYPE)
                                this.FT2232HRadioButton.Checked = true;
                        }
                    }
                }

                if ((ftHandle != IntPtr.Zero) && (ftStatus == FTC_SUCCESS))
                {
                    ftStatus = SPI_InitDevice(ftHandle, MAX_FREQ_93LC56_CLOCK_DIVISOR);

                    if (ftStatus == FTC_SUCCESS)
                    {
                        if ((ftStatus = SPI_GetDeviceLatencyTimer(ftHandle, ref timerValue)) == FTC_SUCCESS)
                        {
                            if ((ftStatus = SPI_SetDeviceLatencyTimer(ftHandle, 50)) == FTC_SUCCESS)
                            {
                                ftStatus = SPI_GetDeviceLatencyTimer(ftHandle, ref timerValue);

                                ftStatus = SPI_SetDeviceLatencyTimer(ftHandle, 16);

                                ftStatus = SPI_GetDeviceLatencyTimer(ftHandle, ref timerValue);
                            }
                        }
                    }

                    if (ftStatus == FTC_SUCCESS)
                    {
                        ftStatus = TestHiSpeedDeviceClock(ftHandle);
                    }

                    if (ftStatus == FTC_SUCCESS)
                    {
                        ftStatus = SPI_SetLoopback(ftHandle, bLoopBackState);
                    }

                    if (ftStatus == FTC_SUCCESS)
                    {
                        // Must set the chip select disable states for all the SPI devices connected to the FT2232C dual device
                        ChipSelectsDisableStates.bADBUS3ChipSelectPinState = false;
                        ChipSelectsDisableStates.bADBUS4GPIOL1PinState = true;
                        ChipSelectsDisableStates.bADBUS5GPIOL2PinState = false;
                        ChipSelectsDisableStates.bADBUS6GPIOL3PinState = true;
                        ChipSelectsDisableStates.bADBUS7GPIOL4PinState = false;

                        // Set the 8 general purpose higher input/output pins
                        HighInputOutputPins.bPin1InputOutputState = false;
                        HighInputOutputPins.bPin1LowHighState = false;
                        HighInputOutputPins.bPin2InputOutputState = false;
                        HighInputOutputPins.bPin2LowHighState = false;
                        HighInputOutputPins.bPin3InputOutputState = true;
                        HighInputOutputPins.bPin3LowHighState = true;
                        HighInputOutputPins.bPin4InputOutputState = false;
                        HighInputOutputPins.bPin4LowHighState = false;
                        HighInputOutputPins.bPin5InputOutputState = false;
                        HighInputOutputPins.bPin5LowHighState = false;
                        HighInputOutputPins.bPin6InputOutputState = false;
                        HighInputOutputPins.bPin6LowHighState = false;
                        HighInputOutputPins.bPin7InputOutputState = false;
                        HighInputOutputPins.bPin7LowHighState = false;
                        HighInputOutputPins.bPin8InputOutputState = false;
                        HighInputOutputPins.bPin8LowHighState = false;
 
                        ftStatus = SPI_SetHiSpeedDeviceGPIOs(ftHandle, ref ChipSelectsDisableStates, ref HighInputOutputPins);
                        if (ftStatus == FTC_SUCCESS)
                        {
                            this.DeviceNameLabel.Text = Device_Name_Label + hiSpeedDeviceName + " Ok";
                            ftHandleA = ftHandle;
                            ChipSelectsDisableStatesA = ChipSelectsDisableStates;
                            HighInputOutputPinsA = HighInputOutputPins;
                        }
                    }
                }

            }
            Console.WriteLine("End");
        }


        private uint TestHiSpeedDeviceClock(IntPtr ftHandle)
        {
            uint ftStatus = FTC_SUCCESS;
            uint clockFrequencyHz = 0;
            if ((ftStatus = SPI_GetHiSpeedDeviceClock(0, ref clockFrequencyHz)) == FTC_SUCCESS)
            {
                if ((ftStatus = SPI_TurnOnDivideByFiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
                {
                    ftStatus = SPI_GetHiSpeedDeviceClock(0, ref clockFrequencyHz);

                    if ((ftStatus = SPI_SetClock(ftHandle, MAX_FREQ_93LC56_CLOCK_DIVISOR, ref clockFrequencyHz)) == FTC_SUCCESS)
                    {
                        ftStatus = SPI_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle);

                        ftStatus = SPI_SetClock(ftHandle, MAX_FREQ_93LC56_CLOCK_DIVISOR, ref clockFrequencyHz);
                    }
                }
            }
            return ftStatus;
        }


        private void set_LED_G()//設備就緒//設備就緒//設備就緒//設備就緒//設備就緒//設備就緒//設備就緒//設備就緒//設備就緒//設備就緒
        {
            uint ftStatus = FTC_SUCCESS;
            HighInputOutputPinsA.bPin5InputOutputState = true;
            HighInputOutputPinsA.bPin5LowHighState = false;
            ftStatus = SPI_SetHiSpeedDeviceGPIOs(ftHandleA, ref ChipSelectsDisableStatesA, ref HighInputOutputPinsA);


            if (Disk_Test)
            {
                WriteLogFile((DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 设备就绪" + "\r\n"), "Log_" + year + week + day + ".txt");
                System.Threading.Thread.Sleep(10);
                //bool RRR = pRef.Compare_file(this.textBox1.Text, pFileItem);
                if ((Disk_Cycle + 1) % (int)(System.Int32.Parse(this.textBox_comapre_stra.Text)) == 0)
                    pCMPSeniorSetting.bNeedCmp = true;
                else
                    pCMPSeniorSetting.bNeedCmp = false;
                pRef.Compare_file(this.textBox1.Text, ref pFileItem);
                pRef.Delete_file(this.textBox1.Text, ref pFileItem);
                setUI();
                int nTotalFile = (int)System.Int32.Parse(this.textBox5.Text) * pFileItem.FList.Count;
                if (System.UInt32.Parse(textBox6.Text) != 0)
                {
                    if (System.UInt32.Parse(textBox6.Text) <= Disk_Cycle)
                    {
                        Disk_Test = false;
                        WriteLogFile("=====================================================" + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("本次测试结束：" + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile(DateTime.Now.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                     
                        WriteLogFile("总周期数目：" + Disk_Cycle.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("总写入档案数目：" + pRef._RealFileNum().ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("Power Up Delay Time : " + Disk_PowerOnTime.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("Power Down Delay Time : " + Disk_PowerOffTime.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("断电随机增加时间 : " + Disk_PowerOffTimeRandom.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                        

                        if (checkBox1.Checked)
                        {
                            WriteLogFile("断电模式：正常" + "\r\n", "Log_" + year + week + day + ".txt");
                        }
                        else
                        {
                            WriteLogFile("断电模式：不正常" + "\r\n", "Log_" + year + week + day + ".txt");
                        }
                        WriteLogFile("每次最大文档数目: " + nTotalFile.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("=====================================================" + "\r\n", "Log_" + year + week + day + ".txt");
                        if (pRef._Disk_Fail())
                        {
                            //this.SetText_Finish("测试失败");
                            this.label2.Text = "测试失败";
                            this.label2.BackColor = Color.Red;
                            //SetBK_Fail_Color_label2(Color.Red);
                            this.textBox8.BackColor = Color.Red;
                            //SetBK_Fail_Color_TextBox8(Color.Red);
                        }
                        else
                        {
                            //this.SetText_Finish("测试成功");
                            this.label2.Text = "测试成功";
                            this.label2.BackColor = Color.Gold;
                            //SetBK_Fail_Color_label2(Color.Gold);
                            this.textBox8.BackColor = Color.Gold;
                            //SetBK_Fail_Color_TextBox8(Color.Gold);
                        }
                    }
                }

                //Disk_WriteTimes = 0;//清除上次寫入次數
                pRef.ClearOneCycle();
                Disk_Cycle++;
               
                if (Disk_Test)
                {
                    pFileItem.ClearAllInfo();
                    if (checkBox1.Checked)
                    {
                        pRef.Copy_file(this.textBox1.Text, ref pFileItem);
                        //Copy_File(Disk_SourceFile, Disk_FileWriteTimes);
                        System.Threading.Thread.Sleep(1000);
                        RunTimer(Disk_PowerOffTime);
                    }
                    else
                    {
                        if (Disk_PowerOffTimeRandom == 0)
                        {
                            RunTimer(Disk_PowerOffTime);
                        }
                        else
                        {
                            Random NUM = new Random();
                            int Random_NUM = NUM.Next(0, Disk_PowerOffTimeRandom);
                            WriteLogFile("本次断电随机增加时间 ms: " + Random_NUM.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                            RunTimer(Disk_PowerOffTime + Random_NUM);
                        }
                        //RunTimer(Disk_PowerOffTime);
                        //myThread_Click();
                        pRef.Copy_file(this.textBox1.Text, ref pFileItem);
                       // Copy_File(Disk_SourceFile, Disk_FileWriteTimes);
                    }
                }

            }
        }
        private void set_LED_R()//設備移除//設備移除//設備移除//設備移除//設備移除//設備移除//設備移除//設備移除//設備移除//設備移除
        {
            uint ftStatus = FTC_SUCCESS;
            HighInputOutputPinsA.bPin5InputOutputState = true;
            HighInputOutputPinsA.bPin5LowHighState = true;
            ftStatus = SPI_SetHiSpeedDeviceGPIOs(ftHandleA, ref ChipSelectsDisableStatesA, ref HighInputOutputPinsA);
            if (Disk_Test)
            {
                WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 设备移出" + "\r\n", "Log_" + year + week + day + ".txt");
                RunTimer(Disk_PowerOnTime);
            }
            //==================test
            //thread1.Abort();

        }



        private string DiskChack(string args)
        {
            //使用IsReady屬性判斷裝置是否就緒//使用IsReady屬性判斷裝置是否就緒//使用IsReady屬性判斷裝置是否就緒//使用IsReady屬性判斷裝置是否就緒
            DriveInfo[] ListDrivesInfo = DriveInfo.GetDrives();
            try
            {
                foreach (DriveInfo vListDrivesInfo in ListDrivesInfo)
                {
                    //使用IsReady屬性判斷裝置是否就緒
                    if (vListDrivesInfo.IsReady)
                    {
                        if (vListDrivesInfo.Name == args)
                        {
                            return "found";
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
            return "Nofound";
        }

        private void button3_Click(object sender, EventArgs e)//啟動測試/啟動測試/啟動測試/啟動測試/啟動測試/啟動測試/啟動測試/啟動測試/啟動測試
        {
            this.TopMost = true;
            if (textBox1.Text =="")
            {
                MessageBox.Show("没有目标盘");
                return;
            }
            DateTime CurrTime = DateTime.Now;
            year = CurrTime.Year.ToString();
            week = CurrTime.Date.Month.ToString();
            day = CurrTime.Date.Day.ToString();

            Disk_PowerOffTime = System.Int32.Parse(textBox3.Text);
            Disk_PowerOnTime = System.Int32.Parse(textBox2.Text);
            Disk_PowerOffTimeRandom = System.Int32.Parse(textBox9.Text);

            this.AddSelFile();

            Disk_SourceFile = textBox4.Text;
            Disk_FileWriteTimes = System.UInt32.Parse(textBox5.Text);

            WriteLogFile("=====================================================" + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("本次测试开始：" + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile(DateTime.Now.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            foreach(string fPath in pFileItem.FList)
            {
                WriteLogFile("写入资料为：" + fPath + "\r\n", "Log_" + year + week + day + ".txt");
            }
            
            WriteLogFile("Power   Up Delya Time : " + Disk_PowerOnTime.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("Power Down Delya Time : " + Disk_PowerOffTime.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("断电随机增加时间为 ms: " + Disk_PowerOffTimeRandom.ToString() + "\r\n", "Log_" + year + week + day + ".txt");

            if (checkBox1.Checked)
            {
                WriteLogFile("断电模式：正常" + "\r\n", "Log_" + year + week + day + ".txt");
            }
            else
            {
                WriteLogFile("断电模式为：不正常" + "\r\n", "Log_" + year + week + day + ".txt");
            }
            WriteLogFile("每次拷贝最大文档数目 : " + ((int)((pFileItem.FList.Count) * (Disk_FileWriteTimes))).ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("=====================================================" + "\r\n", "Log_" + year + week + day + ".txt");
           
            /*bob qiu 2013-10-12*/
            if (pFileItem.FList.Count >= 0)
            {
                Disk_Test = true;
                if (checkBox1.Checked)
                {   
                    pRef.Copy_file(this.textBox1.Text, ref pFileItem);
                    RunTimer(Disk_PowerOffTime);
                }
                else
                {
                    WriteLogFile("本次断电随机增加时间为 ms: " + "0" + "\r\n", "Log_" + year + week + day + ".txt");
                    RunTimer(Disk_PowerOffTime);
                    //myThread_Click();
                    pRef.Copy_file(this.textBox1.Text, ref pFileItem);
                }
            }
            else
            {
                MessageBox.Show("来源档案错误"); 
            }
            /*bob qiu 2013-10-12*/
        }

        private void Copy_File(string _SourceFile, uint Cycle)//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料//寫入資料
        {
            FileStream fs = null;
            FileStream fsout = null;
            //建立緩充串流
            //BufferedStream bs = null;
            //BufferedStream bsout = null;
            try
            {

                //string SourceFile = _SourceFile;
                string TargetFile = textBox1.Text + "\\" + (Disk_FileName + 1).ToString() + ".tmp";
                //FileInfo fInfo = new FileInfo(SourceFile);

               // int BufferSize;

                //byte[] BuffderBytes;

                if (BuffderBytes == null)
                {
                    Console.WriteLine("##############################");
                    Console.WriteLine("##############################");
                    Console.WriteLine("##############################");
                    Console.WriteLine("##############################");
                    Console.WriteLine("##############################");
                    string SourceFile = _SourceFile;
                    FileInfo fInfo = new FileInfo(SourceFile);
                    BufferSize = (Int32)fInfo.Length;
                    BuffderBytes = new byte[BufferSize];
                    fs = File.OpenRead(SourceFile);
                    bs = new BufferedStream(fs, BufferSize);
                    bs.Read(BuffderBytes, 0, BufferSize);

                }
                //if (File.Exists(TargetFile))
                //{
                //    File.Delete(TargetFile);
                //}

                //建立檔案串流
                //fs = File.OpenRead(SourceFile);
                //bs = new BufferedStream(fs, BufferSize);
                //bs.Read(BuffderBytes, 0, BufferSize);

                for (int j = 0; j < Cycle; j++)
                {
                    fsout = File.OpenWrite(textBox1.Text + "\\" + (Disk_FileName + Disk_WriteTimes + 1).ToString() + ".tmp");
                    bsout = new BufferedStream(fsout, BufferSize);
                    bsout.Write(BuffderBytes, 0, BufferSize);
                    fsout.Flush();
                    bsout.Flush();
                    fsout.Dispose();
                    bsout.Dispose();
                    Disk_WriteTimes++;
                    Disk_File_D = true;
                    WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 寫入檔案完成" + Disk_WriteTimes.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                    Console.WriteLine("寫入檔案完成{0}", Disk_WriteTimes);
                }
                bsout.Close();
                
            }
            catch (Exception ex)
            {
                if (Disk_WriteTimes > 0)
                {
                    Disk_File_D = true;
                }
                else
                {
                    Disk_FileName++;
                    Disk_File_D = false;
                }
                WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 檔案完成" + Disk_WriteTimes.ToString() + "次 " + "\r\n", "Log_" + year + week + day + ".txt");
                Console.WriteLine("檔案完成{0}次", Disk_WriteTimes);
                Console.WriteLine("資料寫入的錯誤!!!");
                Console.WriteLine(ex.Message);
            }
            finally
            {
                if (fs != null)
                    fs.Dispose();
                if (fsout != null)
                    fsout.Dispose();
                //if (bs != null)
                //    bs.Dispose();
                //if (bsout != null)
                //    bsout.Close();
            }


        }

        private void button1_Click(object sender, EventArgs e)
        {
            Open_FD2232H();
        }

        private bool CopyCompare(string SourceFile, uint TargetFileS, uint TargetFileT)//複製比對//複製比對//複製比對//複製比對//複製比對//複製比對//複製比對//複製比對//複製比對
        {
            bool freturt =true;
            int fCopyCompare = 0;
            try
            {
                byte[] bufferS = new byte[10];
                byte[] bufferT = new byte[10];
                bool File_L;
                string outputS;
                string outputT;
                if (Disk_File_D == true)
                {
                    WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 資料有完成" + TargetFileT.ToString() + "次" + "\r\n", "Log_" + year + week + day + ".txt");
                    Console.WriteLine("資料有完成{0}次", TargetFileT);
                }
                else
                {
                    WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 資料沒有半個完成，不比較" + "\r\n", "Log_" + year + week + day + ".txt");
                    Console.WriteLine("資料沒有半個完成，不比較");
                    return true;
                }

                if (File.Exists(SourceFile))
                {
                }
                else
                {
                    Console.WriteLine("沒有來源檔");
                    Disk_WriteTimes = 0;
                    Disk_Test = false;//停止測試
                    this.label8.Text = "沒有來源檔，停止測試，不要問我為啥= =";
                    this.label9.Text = "沒有來源檔，停止測試，不要問我為啥= =";
                    this.label10.Text = "沒有來源檔，停止測試，不要問我為啥= =";
                    return false;
                }


                for (int j = 1; j < TargetFileT + 1; j++)
                {
                    fCopyCompare = j;
                    if (File.Exists(textBox1.Text + "\\" + (TargetFileS + j).ToString() + ".tmp"))
                    {
                        Console.WriteLine("檢查檔案{0}存在", TargetFileS + j);
                        WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 檢查檔案：" + (TargetFileS + j).ToString() + "存在" + "\r\n", "Log_" + year + week + day + ".txt");
                    }
                    else
                    {
                        //freturt = false;
                        Console.WriteLine("沒有目的檔{0}", TargetFileS + j);
                        WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 沒有目的檔" + (TargetFileS + j).ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                    }
                    FileInfo fInfoS = new FileInfo(SourceFile);
                    FileInfo fInfoT = new FileInfo(textBox1.Text + "\\" + (TargetFileS + j).ToString() + ".tmp");

                    if (fInfoS.Length == fInfoT.Length)
                    {
                        Console.WriteLine("檔案{0}比較長度相同", TargetFileS + j);
                        File_L = true;
                        WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 檔案" + (TargetFileS + j).ToString() + "比較長度相同" + "\r\n", "Log_" + year + week + day + ".txt");
                    }
                    else
                    {
                        File_L = false;
                        //freturt = false;
                        Console.WriteLine("檔案{0}比較長度不相同", TargetFileS + j);
                        WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 檔案" + (TargetFileS + j).ToString() + "比較長度不相同" + "\r\n", "Log_" + year + week + day + ".txt");
                        WriteLogFile("檔案長度：" + fInfoT.Length.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
                    }


                    if (File_L)
                    {
                        FileStream MyFileS = new FileStream(SourceFile, FileMode.Open);
                        FileStream MyFileT = new FileStream(textBox1.Text + "\\" + (TargetFileS + j).ToString() + ".tmp", FileMode.Open);
                        MyFileS.Seek(((Int32)fInfoS.Length - 10), SeekOrigin.Begin);
                        MyFileT.Seek(((Int32)fInfoT.Length - 10), SeekOrigin.Begin);


                        MyFileS.Read(bufferS, 0, 10);
                        outputS = System.Text.Encoding.ASCII.GetString(bufferS);

                        MyFileT.Read(bufferT, 0, 10);
                        outputT = System.Text.Encoding.ASCII.GetString(bufferT);


                        if (outputS == outputT)
                        {
                            Console.WriteLine("檔案{0}比較資料相同", TargetFileS + j);
                            WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 檔案" + (TargetFileS + j).ToString() + "比較資料相同" + "\r\n", "Log_" + year + week + day + ".txt");
                        }
                        else
                        {
                            freturt = false;
                            if (fCopyCompare == TargetFileT)
                            {
                                freturt = true;
                            }
                            Console.WriteLine("檔案{0}比較資料不相同", TargetFileS + j);
                            WriteLogFile("檔案資料結尾10Bytes：\"" + outputT + "\"\r\n", "Log_" + year + week + day + ".txt");
                            Console.WriteLine("檔案資料結尾10Bytes：\"" + outputT + "\"\r\n");
                            MyFileT.Seek(0, SeekOrigin.Begin);
                            MyFileT.Read(bufferT, 0, 10);
                            outputT = System.Text.Encoding.ASCII.GetString(bufferT);
                            WriteLogFile("檔案資料開頭10Bytes：\"" + outputT + "\"\r\n", "Log_" + year + week + day + ".txt");
                            Console.WriteLine("檔案資料開頭10Bytes：\"" + outputT + "\"\r\n");
                            WriteLogFile(DateTime.Now.ToString() + "  " + sw.ElapsedMilliseconds.ToString("0#####") + " 檔案" + (TargetFileS + j).ToString() + "比較資料不相同" + "\r\n", "Log_" + year + week + day + ".txt");
                        }
                        MyFileS.Close();
                        MyFileT.Close();
                        MyFileS.Dispose();
                        MyFileT.Dispose();
                    }
                    else
                    {
                        if (fCopyCompare == TargetFileT)
                        {
                            //freturt = true;
                            Console.WriteLine("斷電的關西？");
                        }
                    }
                }
                return freturt;

            }
            catch (Exception ex)
            {
                Console.WriteLine("CompareFile{0}", fCopyCompare);
                Console.WriteLine("資料比對的錯誤!!!");
                Console.WriteLine(ex.Message);
                Disk_Exception = "\"" +ex.Message+ "\"" + "本次第" + fCopyCompare.ToString();
                return false;

            }


        }

        private void button2_Click(object sender, EventArgs e)
        {
            //Console.WriteLine(DateTime.Now.ToString());
            //Console.WriteLine(System.DateTime.Today.ToShortDateString());
            //WriteLogFile("acb\r\nacb\r\nacb\r\nacb\r\nacb\r\nacb\r\nacb\r\nacb\r\n", "D://Log.txt");
            Disk_Test = false;//停止測試
            pRef._Disk_PowerOff(true);
            this.TopMost = false;
            //string[] DriveList = Environment.GetLogicalDrives();
            //comboBox1.Items.Clear();
            //comboBox1.Items.AddRange(DriveList);

        }



        private void WriteLogFile(string msg, string TargetFile)//寫LOG//寫LOG檔//寫LOG檔//寫LOG檔//寫LOG檔//寫LOG檔//寫LOG檔
        {
            lock (this)
            {
                byte[] buffer;
                FileStream MyFile = new FileStream(textBox7.Text + TargetFile, FileMode.Append);
                //buffer = System.Text.Encoding.Unicode.GetBytes(msg);
                buffer = System.Text.Encoding.Default.GetBytes(msg);
                //buffer = System.Text.Encoding.ASCII.GetBytes(msg);
                //MyFile.Write(buffer, 0, msg.Length);
                MyFile.Write(buffer, 0, buffer.GetLength(0));
                MyFile.Close();
                this.output.WriteLine(msg);
            }
        }
        delegate void SetTextCallback(string text); 
        
        private void SetText_Current_FileNUM(string text)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.label7.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText_Current_FileNUM);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                this.label7.Text = text;
            }
        }

        private void SetText_Cur_Cycle(string text)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.label8.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText_Cur_Cycle);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                this.label8.Text = text;
            }
        }
        private void SetText_Finish(string text)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.label2.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText_Cur_Cycle);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                this.label2.Text = text;
            }
        }
         private void SetText_Current_RealFile_Num(string text)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.label9.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(SetText_Current_RealFile_Num);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                this.label9.Text = text;
            }
        }

         private void SetText_Current_Err_Info(string text)
         {
             // InvokeRequired required compares the thread ID of the 
             // calling thread to the thread ID of the creating thread. 
             // If these threads are different, it returns true. 
             if (this.label10.InvokeRequired)
             {
                 SetTextCallback d = new SetTextCallback(SetText_Current_Err_Info);
                 this.Invoke(d, new object[] { text });
             }
             else
             {
                 this.label10.Text = text;
             }
         }



        private void setUI()//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI//改UI
        {
            int nShouldTotalFileNum = 0;
            if (System.Int32.Parse(this.textBox6.Text) != 0)
            {
                nShouldTotalFileNum = System.Int32.Parse(this.textBox6.Text) * (int)(pFileItem.FList.Count) * (int)System.Int32.Parse(this.textBox5.Text);
                WriteLogFile("应该写入总文档数：" + nShouldTotalFileNum.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            }
            else
                WriteLogFile("无限循环文档数目为无限\r\n", "Log_" + year + week + day + ".txt");
                //int nErrorNum = pRef._FileFailCMP() + pRef._FileLostNum();
                //WriteLogFile(DateTime.Now.ToString() + "\r\n", "Log_" + year + week + day + ".txt");

            WriteLogFile("当前周期数：" + Disk_Cycle.ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("当前周期写入文档数目：" + pRef._Cycle_File_Num().ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("丢失文件+写错的文件数目：" + pRef._FileLostNum().ToString() + "+" + pRef._FileFailCMP().ToString() + "\r\n", "Log_" + year + week + day + ".txt");
            if(this.checkBox_pre_write_disk.Checked)
                WriteLogFile("预处理文件错误：" + pRef._FILE_CMP_Senior() + "\r\n", "Log_" + year + week + day + ".txt");
            WriteLogFile("-----------------------------------------------------" + "\r\n", "Log_" + year + week + day + ".txt");
            int nTotal = (int)System.Int32.Parse(this.textBox5.Text) * pFileItem.FList.Count;
            //this.SetText_Current_FileNUM("当前周期应写入文档数目：" + nTotal.ToString());
            this.label7.Text = "当前周期应写入文档数目：" + nTotal.ToString();
            //this.SetText_Cur_Cycle("当前周期数：" + Disk_Cycle.ToString());
            this.label8.Text = "当前周期数：" + Disk_Cycle.ToString();
            //this.SetText_Current_RealFile_Num("当前周期写入文档数目：" + pRef._Cycle_File_Num().ToString());
            this.label9.Text = "当前周期写入文档数目：" + pRef._Cycle_File_Num().ToString();
            
            if (pRef._Disk_Fail())
            {
                this.label10.BackColor = Color.Red;
                //SetBK_Fail_Color(Color.Red);
            }
            //this.SetText_Current_Err_Info("丢失文件+写错的文件数目：" + pRef._FileLostNum().ToString() + "+" + pRef._FileFailCMP().ToString());
            this.label10.Text = "丢失文件+写错的文件数目：" + pRef._FileLostNum().ToString() + "+" + pRef._FileFailCMP().ToString();
        }


        delegate void SetBKCallBack(Color color);
        private void SetBK_Fail_Color(Color color)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.label10.InvokeRequired)
            {
                SetBKCallBack d = new SetBKCallBack(SetBK_Fail_Color);
                this.Invoke(d, new object[] { color });
            }
            else
            {
                this.label10.BackColor = color;
            }
        }

        private void SetBK_Fail_Color_label2(Color color)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.label2.InvokeRequired)
            {
                SetBKCallBack d = new SetBKCallBack(SetBK_Fail_Color_label2);
                this.Invoke(d, new object[] { color });
            }
            else
            {
                this.label2.BackColor = color;
            }
        }

        private void SetBK_Fail_Color_TextBox8(Color color)
        {
            // InvokeRequired required compares the thread ID of the 
            // calling thread to the thread ID of the creating thread. 
            // If these threads are different, it returns true. 
            if (this.textBox8.InvokeRequired)
            {
                SetBKCallBack d = new SetBKCallBack(SetBK_Fail_Color_TextBox8);
                this.Invoke(d, new object[] { color });
            }
            else
            {
                this.textBox8.BackColor = color;
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            uint ftStatus;
            if (HighInputOutputPinsA.bPin3LowHighState)
            {
                if (pRef != null)
                    pRef._Disk_PowerOff(true);
                HighInputOutputPinsA.bPin3LowHighState = false;
            }
            else
            {
                HighInputOutputPinsA.bPin3LowHighState = true;
                if (pRef != null)
                    pRef._Disk_PowerOff(false);
            }
            ftStatus = SPI_SetHiSpeedDeviceGPIOs(ftHandleA, ref ChipSelectsDisableStatesA, ref HighInputOutputPinsA);
           // while (ftStatus) {Console.Write("waiting for the setting succesfull\n" };
        }

        private void button5_Click_1(object sender, EventArgs e)
        {
            folderBrowserDialog1.ShowDialog();
            textBox7.Text = this.folderBrowserDialog1.SelectedPath + Path.GetFileName(textBox7.Text);
        }

        private void button6_Click(object sender, EventArgs e)
        {
            Disk_SourceFile = textBox4.Text;
            openFileDialog1.InitialDirectory = Disk_SourceFile;
            this.openFileDialog1.ShowDialog();
            textBox4.Text = this.openFileDialog1.FileName;
        }

        private void myThread_Click()
        {
            Disk_SourceFile = textBox4.Text;
            Disk_FileWriteTimes = System.UInt32.Parse(textBox5.Text);
            ThreadStart myRun = delegate { Copy_File(Disk_SourceFile, Disk_FileWriteTimes); };
            //2.建立Thread 類別
            thread1 = new Thread(myRun);
            thread1.IsBackground = true;
            //3.啟動執行緒
            thread1.Start();
           
        }

        private void button7_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Multiselect = true;
            int nLength = 0;
           // string strfName;
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                foreach(string strfName in ofd.FileNames)
                {
                    nLength++;
                    Console.WriteLine("{0}",strfName);
                    pFileItem.FList.Add(strfName);
                }

                this.UpdateListViewUI(ref pFileItem.FList, nLength);
            }
        }
        private void UpdateListHeader()
        {
            ColumnHeader  ch= new ColumnHeader();  
      
            ch.Text = "源文件";   //设置列标题   
            ch.Width = 120;    //设置列宽度   
            ch.TextAlign = HorizontalAlignment.Left;   //设置列的对齐方式   
            this.SelFileList.Columns.Add(ch);    //将列头添加到ListView控件。  

            ColumnHeader ch2 = new ColumnHeader();  
            ch2.Text = "目标文件位置";
            ch2.Width = 120;
            ch2.TextAlign = HorizontalAlignment.Right;
           
            this.SelFileList.Columns.Add(ch2);    //将列头添加到ListView控件。  
        }
        private void UpdateListViewUI(ref List<string> lFilePath,int uHowManyDate)
        {
            this.SelFileList.BeginUpdate();
            //this.SelFileList.Clear();
            this.SelFileList.Items.Clear();
            for (int i = 0; i < uHowManyDate; i++)   
            {
                ListViewItem lvi = new ListViewItem();
                lvi.Text = lFilePath[i];
                lvi.SubItems.Add(this.textBox1.Text);
                this.SelFileList.Items.Add(lvi);
            }
            this.SelFileList.EndUpdate();
        }

        private void TMethodTextChanged(object sender, EventArgs e)
        {
            if (this.TMethodCombox.Text == "Medium Test")
            {
                //pRef = new WindowsFormsApplication4.CMediumTMethod();
            }
            else if (this.TMethodCombox.Text == "Senior Test")
            {
                //pRef = new WindowsFormsApplication4.CMediumTMethod();
                //pRef.Copy_file(this.textBox1.Text);
            }
            else
            {
                //pRef = new WindowsFormsApplication4.CMediumTMethod();
            }
        }

        private void AddSelFile()
        {
            if (this.checkBox_pre_write_disk.Checked)
                pRef = new CSeniorTMethod(System.Int32.Parse(this.textBox5.Text), ref pCMPSetting, ref pCMPSeniorSetting);
            else
                pRef = new CMediumTMethod(System.Int32.Parse(this.textBox5.Text), ref pCMPSetting);

            pCMPSeniorSetting.bNeedCmp = false;
            pCMPSeniorSetting.ulSize = (long)System.Int64.Parse(this.textBox_percent.Text);

            pCMPSetting.bEnableAll = this.Enable_All_CMP.Checked;
            pCMPSetting.ulCmpSize = (long)System.Int64.Parse(this.textBox_compare_byte.Text);

            pSettingItem.num_big = 1;

            pSettingItem.num_medium = System.Int32.Parse(this.textBox_num_medium.Text);
            pSettingItem.num_small = System.Int32.Parse(this.textBox_num_small.Text);

            pSettingItem.size_big = System.Int64.Parse(this.textBox_size_big.Text);
            pSettingItem.size_medium = System.Int64.Parse(this.textBox_size_medium.Text);
            pSettingItem.size_small = System.Int64.Parse(this.textBox_size_small.Text);

            pSettingItem.value_big = this.com_value_big.SelectedIndex;
            pSettingItem.value_medium = this.com_value_medium.SelectedIndex;
            pSettingItem.value_small = this.com_value_small.SelectedIndex;
            pSettingItem.bEnableSmallFile = this.checkBox_enable_small.Checked;
            pSettingItem.bEnableBigFile = this.checkBox_enable_big.Checked;
            pSettingItem.bEnableMediumFile = this.checkBox_enable_medium.Checked;
            string strTargetPath = this.textBox1.Text;
            pRef.Create_file(ref strTargetPath, ref pSettingItem);
            string strCurPath = System.IO.Directory.GetCurrentDirectory();
            string strBigSize, strMediumSize, strSmallSize;
            string strBigNum, strMediumNum, strSmallNum;
            strBigSize = pSettingItem.size_big.ToString();
            strBigNum = pSettingItem.num_big.ToString();
            strMediumSize = pSettingItem.size_medium.ToString();
            strMediumNum = pSettingItem.num_medium.ToString();
            strSmallSize = pSettingItem.size_small.ToString();
            strSmallNum = pSettingItem.num_small.ToString();


            if (strCurPath[strCurPath.Length - 1] != '\\') //非根目录
                strCurPath += "\\";
            string strOperPath = "";
            pFileItem.FList.Clear();
            if (pSettingItem.bEnableBigFile)
            {
                strOperPath = strCurPath + strBigSize + "G" + "\\" + this.com_value_big.Text+"\\";
                int nBigFileNum = pSettingItem.num_big, nBigFileIndex = 0;
                for (nBigFileIndex = 1; nBigFileIndex <= nBigFileNum; nBigFileIndex++)
                    pFileItem.FList.Add(strOperPath + nBigFileNum.ToString() + ".srctesttmp");
            }
            if (pSettingItem.bEnableMediumFile)
            {
                strOperPath = strCurPath + strMediumSize + "M" + "\\" + this.com_value_medium.Text + "\\";
                int nMediumFileNum = pSettingItem.num_medium, nMediumFileIndex = 0;
                for (nMediumFileIndex = 1; nMediumFileIndex <= nMediumFileNum; nMediumFileIndex++)
                    pFileItem.FList.Add(strOperPath + nMediumFileIndex.ToString() + ".srctesttmp");
            }
            if (pSettingItem.bEnableSmallFile)
            {
                strOperPath = strCurPath + strSmallSize + "K" + "\\" + this.com_value_big.Text + "\\";
                int nSmallFileNum = pSettingItem.num_small, nSmallFileIndex = 0;
                for (nSmallFileIndex = 1; nSmallFileIndex <= nSmallFileNum; nSmallFileIndex++)
                    pFileItem.FList.Add(strOperPath + nSmallFileIndex.ToString() + ".srctesttmp");
            }
        }

        private void button7_Click_1(object sender, EventArgs e)
        {
            //ref CFileItem refFI, int nIndex, string TargetFPath, int nHowManyTimes
            //string strDstName = pRef.GetDstFullName();

            //pRef.Create_file(ref pSettingItem);
            //pRef.Delete_file(this.textBox1.Text, ref pFileItem);
        }
    }
}
