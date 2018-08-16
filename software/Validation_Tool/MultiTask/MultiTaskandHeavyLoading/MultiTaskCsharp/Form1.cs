using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO;
using System.Threading;
using System.Diagnostics; 

namespace MultiTaskCsharp
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            byte i;
            System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = false;
            InitializeComponent();
            for (i = 1; i <= 10;i++ )
                GetAllDriversType(i);
           

        }
        
        
        public struct UITASKPARAMETER
        {
            public string SourceDisk;
            public string DestDisk;
            public byte Pattern;
            public byte FileSize;
            public string Time;
            public string Selected;
            public string Log_hFile;
        }
        public UITASKPARAMETER[] uitaskparameter = new UITASKPARAMETER[10];

        public int Test_Method, Copy_Method, Test_Function;

        public IntPtr[] File_src = new IntPtr[10];
        public IntPtr[] File_dest = new IntPtr[10];

        public Thread[] Task = new Thread[10];

        public struct DataCheck
        {
            public string FileSize;
            public string FileNumber;
            public UInt64 StartLba;
        }
        public DataCheck[,] datacheck = new DataCheck[10,300];
        public DataCheck[] datacheck_HL = new DataCheck[300];
        

        public Form2 form2;
        public Form3 form3;
        public Form4 form4;
        public Form5 form5;
        public Form6 form6;
        public Form7 form7;
        public Form8 form8;
        public Form9 form9;
        public Form10 form10;
        public Form11 form11;



        [DllImport("kernel32.dll")]
        public static extern IntPtr CreateFile
            (
            string lpFileName,
            uint dwDesiredAccess,
            uint dwShareMode,
            uint lpSecurityAttributes,
            uint dwCreationDisposition,
            uint dwFlagsAndAttributes,
            uint hTemplateFile
            );
        [DllImport("kernel32.dll")]
        public static extern IntPtr CopyFileA
            (
            string FileName_Src,
            string FileName_Dest,
            bool LP
            );

        /*[DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern bool WriteFile
            (
            IntPtr lpFileName,
            //string lpFileName,
            //System.Text.StringBuilder lpBuffer,
            Byte[] lpBuffer,
            uint nNumberOfBytesToWrite,
            out uint lpNumberOfBytesWritten,
            uint lpOverlapped
            );*/

        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern bool WriteFile
            (
            IntPtr lpFileName,
            //string lpFileName,
            //System.Text.StringBuilder lpBuffer,
            UInt64[] lpBuffer,
            uint nNumberOfBytesToWrite,
            out uint lpNumberOfBytesWritten,
            uint lpOverlapped
            );
        /*[DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern bool ReadFile
            (
            IntPtr lpFileName,
            //string lpFileName,
            //System.Text.StringBuilder lpBuffer,
            Byte[] lpBuffer,
            uint nNumberOfBytesToRead,
            out uint lpNumberOfBytesRead,
            uint lpOverlapped
            );*/
        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern bool ReadFile
            (
            IntPtr lpFileName,
            //string lpFileName,
            //System.Text.StringBuilder lpBuffer,
            UInt64[] lpBuffer,
            uint nNumberOfBytesToRead,
            out uint lpNumberOfBytesRead,
            uint lpOverlapped
            );

        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern UInt32 SetFilePointer
            (
            IntPtr lpFileName,
            UInt32 lDistanceToMove,
            UInt32 lpDistanceToMoveHigh, // 偏移量(高位)
            UInt32 dwMoveMethod
            );

        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern UInt32 RemoveDirectory
            (
            string LpPathName
            );

      
        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern bool CloseHandle
            (
               IntPtr hobject
            );
        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern uint GetDriveType
            (
               string lpRootPathName   // root directory
            );
        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern int GetLogicalDrives();

        [DllImport("kernel32")]
        private static extern bool WritePrivateProfileString
            (
            string lpAppName,
            string lpKeyName,
            string lpString,
            string lpFileName
            );

        [DllImport("kernel32")]
        private static extern int GetPrivateProfileInt
            (
            string lpAppName,
            string lpKeyName,
            int nDefault,
            string lpFileName
            );

        [DllImport("kernel32")]
        private static extern int GetPrivateProfileString
            (
            string lpAppName,
            string lpKeyName,
            string lpDefault,
            StringBuilder lpReturnedString,
            int nSize,
            string lpFileName
            ); 
 
        [Serializable,  
        System.Runtime.InteropServices.StructLayout  
       (System.Runtime.InteropServices.LayoutKind.Sequential,  
        CharSet = System.Runtime.InteropServices.CharSet.Auto  
         ),  
        System.Runtime.InteropServices.BestFitMapping(false)]  
        public struct WIN32_FIND_DATA  
        {  
            public int dwFileAttributes;  
            public int ftCreationTime_dwLowDateTime;  
            public int ftCreationTime_dwHighDateTime;  
            public int ftLastAccessTime_dwLowDateTime;  
            public int ftLastAccessTime_dwHighDateTime;  
            public int ftLastWriteTime_dwLowDateTime;  
            public int ftLastWriteTime_dwHighDateTime;  
            public int nFileSizeHigh;  
            public int nFileSizeLow;  
            public int dwReserved0;  
            public int dwReserved1;  
                [System.Runtime.InteropServices.MarshalAs  
              (System.Runtime.InteropServices.UnmanagedType.ByValTStr,  
              SizeConst = 260)]  
            public string cFileName;  
            [System.Runtime.InteropServices.MarshalAs  
              (System.Runtime.InteropServices.UnmanagedType.ByValTStr,  
              SizeConst = 14)]  
            public string cAlternateFileName;  
        }  
        [System.Runtime.InteropServices.DllImport  
          ("kernel32.dll",  
          CharSet = System.Runtime.InteropServices.CharSet.Auto,  
          SetLastError = true)]  
        private static extern IntPtr FindFirstFile(string pFileName, ref WIN32_FIND_DATA pFindFileData);  
        [System.Runtime.InteropServices.DllImport  
          ("kernel32.dll",  
          CharSet = System.Runtime.InteropServices.CharSet.Auto,  
          SetLastError = true)]  
        private static extern bool FindNextFile(IntPtr hndFindFile, ref WIN32_FIND_DATA lpFindFileData);  
        [System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true)]  
        private static extern bool FindClose(IntPtr hndFindFile);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool GetDiskFreeSpaceEx(string lpDirectoryName,
        out ulong lpFreeBytesAvailable,
        out ulong lpTotalNumberOfBytes,
        out ulong lpTotalNumberOfFreeBytes);

        private static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);
        //WIN32_FIND_DATA FindFileData;

        /*[DllImport("kernel32")]
        static extern bool GetDiskFreeSpaceEx(
        string lpDirectoryName,
        ref ulong lpFreeBytesAvailable,
        ref ulong lpTotalNumberOfBytes,
        ref ulong lpTotalNumberOfFreeBytes);*/

        [DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int SHFileOperation(SHFILEOPSTRUCT lpFileOp);
    
        private const string FILE_SPLITER = "/0";
    ///
    /// Shell文件操作数据类型
    ///
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private class SHFILEOPSTRUCT
        {
            public IntPtr hwnd;
           
            public wFunc wFunc;
            
            public string pFrom;
           
            public string pTo;
            
            public FILEOP_FLAGS fFlags;
            
            public bool fAnyOperationsAborted;
            public IntPtr hNameMappings;
            
            public string lpszProgressTitle;
        }
        ///
        /// 文件操作方式
        ///
        private enum wFunc
        {
            
            FO_MOVE = 0x0001,
           
            FO_COPY = 0x0002,
            
            FO_DELETE = 0x0003,
           
            FO_RENAME = 0x0004
        }
    ///
    /// fFlags枚举值，
    /// 参见：http://msdn.microsoft.com/zh-cn/library/bb759795（v=vs.85）。aspx
    ///
        private enum FILEOP_FLAGS
        {
            ///
            ///pTo 指定了多个目标文件，而不是单个目录
            ///The pTo member specifies multiple destination files （one for each source file） rather than one directory where all source files are to be deposited.
            ///
            FOF_MULTIDESTFILES = 0x1,
           
            FOF_CONFIRMMOUSE = 0x2,
            
            FOF_SILENT = 0x4,
           
            FOF_RENAMEONCOLLISION = 0x8,
           
            FOF_NOCONFIRMATION = 0x10,
           
            FOF_WANTMAPPINGHANDLE = 0x20,
            
            FOF_ALLOWUNDO = 0x40,
           
            FOF_FILESONLY = 0x80,
          
            FOF_SIMPLEPROGRESS = 0x100,
           
            FOF_NOCONFIRMMKDIR = 0x200,
            
            FOF_NOERRORUI = 0x400,
           
            FOF_NOCOPYSECURITYATTRIBS = 0x800,
           
            FOF_NORECURSION = 0x1000,
            
            FOF_NO_CONNECTED_ELEMENTS = 0x2000,
            
            FOF_WANTNUKEWARNING = 0x4000,
           
            FOF_NORECURSEREPARSE = 0x8000,
        }
        //API函数声明

        private const uint FILE_SHARE_READ = 0x00000001;
        private const uint FILE_SHARE_WRITE = 0x00000002;
        private const uint DELETE = 0x00010000;
        private const uint FILE_SHARE_DELETE = 0x00000004;
        private const uint FILE_ATTRIBUTE_NORMAL = 0x00000080;
        private const uint GENERIC_READ = 0x80000000;
        private const uint GENERIC_WRITE = 0x40000000;
        private const uint CREATE_ALWAYS = 2;
        private const uint FILE_FLAG_DELETE_ON_CLOSE = 0x04000000;
        private const uint OPEN_EXISTING = 3;
        private const uint FILE_FLAG_NO_BUFFERING = 0x20000000;
        private const uint FILE_FLAG_WRITE_THROUGH = 0x80000000; 
        private const uint FILE_ATTRIBUTE_DIRECTORY = 0x00000010;

        private const UInt32 FILE_BEGIN = 0;
        private const UInt32 FILE_CURRENT = 1;
        private const UInt32 FILE_END = 2;



        //public enum DriveType { DRIVE_UNKNOWN = 0, DRIVE_NO_ROOT_DIR = 1, DRIVE_REMOVABLE = 2, DRIVE_FIXED = 3, DRIVE_REMOTE = 4, DRIVE_CDROM = 5, DRIVE_RAMDISK = 6 } 
        private const uint DRIVE_UNKNOWN = 0;
        private const uint DRIVE_NO_ROOT_DIR = 1;
        private const uint DRIVE_REMOVABLE = 2;
        private const uint DRIVE_FIXED = 3;
        private const uint DRIVE_REMOTE = 4;
        private const uint DRIVE_CDROM = 5;
        private const uint DRIVE_RAMDISK = 6;

        

        public static string CheckDir(string strPath)
        {
            if (!Directory.Exists(strPath))
            {
                Directory.CreateDirectory(strPath);
            }
            return strPath;
        }                              //创建文件夹函数声明



        public void GetAllDriversType(byte Ti)
        {
            ComboBox combobox_src, combobox_dest;
            int GetDrive;
            int count = 0, i = 1;
            string GetType = "";
            char DriverName = 'x';
            string[] AllDrives = { "","","","","","","","","","","","","","","","" };//长度16
            GetDrive = GetLogicalDrives();
            combobox_src = (ComboBox)Controls.Find("T" + Ti.ToString() + "_Src", true)[0];
            combobox_dest = (ComboBox)Controls.Find("T" + Ti.ToString() + "_Dest", true)[0];
            while (GetDrive != 0)
            {
                if ((GetDrive & 1) != 0)
                {
                    DriverName = (char)(i + 64);//存在的驱动器名称
                    GetType = GetOneDriverType(DriverName);//存在的驱动器类型
                    AllDrives[count] = DriverName.ToString() + ":" + GetType;
                    //WriteLogFile(DriverName.ToString() + ":" + GetType + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                          
                    if ((GetType == "Local Drive") || (GetType == "Removable Drive"))
                    {
                        combobox_src.Items.Add(AllDrives[count]);
                        combobox_dest.Items.Add(AllDrives[count]);
                    }
                    count++;//存在的驱动器个数
                }
                GetDrive >>= 1;
                i++;
            }

        }
        public string GetOneDriverType(char DriverName)
        {
            string typename;
            char[] PathNameC;
            string RootPathName =  "x:\\";
            PathNameC = RootPathName.ToCharArray();
            PathNameC[0] = DriverName;
            RootPathName = new string(PathNameC);
            uint type = GetDriveType(RootPathName);
            switch (type)
            {
                case DRIVE_UNKNOWN:
                    typename = "Unknown Drive";
                    break;
                case DRIVE_NO_ROOT_DIR:
                    typename = "No Root Dir Drive";
                    break;
                case DRIVE_REMOVABLE:
                    typename = "Removable Drive";
                    break;
                case DRIVE_FIXED:
                    typename = "Local Drive";
                    break;
                case DRIVE_REMOTE:
                    typename = "Remote Driver";
                    break;
                case DRIVE_CDROM:
                    typename = "CD-ROM";
                    break;
                case DRIVE_RAMDISK:
                    typename = "RAM";
                    break;
                default:
                    typename = "Unknown Drive";
                    break;
            }
            return typename;
        }



            public void GetUIMsg()
            {

                //GlobalVariables.i = 1;
                byte i = 1;
                int j = 0;
                CheckBox checkbox;
                //RadioButton radiobutton;
                TextBox textbox;
                ComboBox combobox;
                char[] Arry_path;

                

                while (i <= 10)
                {
                    checkbox = this.Controls.Find("T" + i.ToString() + "_Select", true)[0] as CheckBox;
                    if (checkbox.Checked == true)
                    {
                        uitaskparameter[i - 1].Selected = "enable";
                        combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Src", true)[0];

                        if(combobox.SelectedIndex == -1)
                        {
                            uitaskparameter[i - 1].SourceDisk = "D";
                        }
                        else
                        {
                            Arry_path = combobox.Text.ToCharArray();
                            uitaskparameter[i - 1].SourceDisk = Arry_path[0].ToString();
                        }
                        
                        combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Dest", true)[0];
                        if (combobox.SelectedIndex == -1)
                            uitaskparameter[i - 1].DestDisk = "E";
                        else
                        {
                            Arry_path = combobox.Text.ToCharArray();
                            uitaskparameter[i - 1].DestDisk = Arry_path[0].ToString();
                        }
                            
                        combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Pattern", true)[0];
                        if (combobox.SelectedIndex == 0)
                            uitaskparameter[i - 1].Pattern = 1;
                        else if (combobox.SelectedIndex == 1)
                            uitaskparameter[i - 1].Pattern = 2;
                        else if (combobox.SelectedIndex == 2)
                            uitaskparameter[i - 1].Pattern = 3;
                        else if (combobox.SelectedIndex == 3)
                            uitaskparameter[i - 1].Pattern = 4;
                        else if (combobox.SelectedIndex == 4)
                            uitaskparameter[i - 1].Pattern = 5;
                        else
                            uitaskparameter[i - 1].Pattern = 1;

                        combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_FileSize", true)[0];
                        if (combobox.SelectedIndex == 0)
                            uitaskparameter[i - 1].FileSize = 1;
                        else if (combobox.SelectedIndex == 1)
                            uitaskparameter[i - 1].FileSize = 2;
                        else if (combobox.SelectedIndex == 2)
                            uitaskparameter[i - 1].FileSize = 3;
                        else if (combobox.SelectedIndex == 3)
                            uitaskparameter[i - 1].FileSize = 4;
                        else if (combobox.SelectedIndex == 4)
                            uitaskparameter[i - 1].FileSize = 5;
                        else if (combobox.SelectedIndex == 5)
                            uitaskparameter[i - 1].FileSize = 6;
                        else if (combobox.SelectedIndex == 6)
                            uitaskparameter[i - 1].FileSize = 7;
                        else if (combobox.SelectedIndex == 7)
                            uitaskparameter[i - 1].FileSize = 8;
                        else
                            uitaskparameter[i - 1].FileSize = 3;

                        textbox = (TextBox)this.Controls.Find("T" + i.ToString() + "_Time", true)[0];
                        uitaskparameter[i - 1].Time = textbox.Text;
                        if (uitaskparameter[i - 1].Time == "")
                            uitaskparameter[i - 1].Time = "10";
                        i++;
                    }
                    else
                    {
                        uitaskparameter[i - 1].Selected = "disable";
                        i++;
                        j++;
                    }
                }
                if (j == 10)
                {
                    uitaskparameter[j - 1].Selected = "enable";
                    uitaskparameter[j - 1].SourceDisk = "D";
                    uitaskparameter[j - 1].DestDisk = "E";
                    uitaskparameter[j - 1].Pattern = 1;
                    uitaskparameter[j - 1].FileSize = 1;
                    uitaskparameter[j - 1].Time = "10";
                }

                combobox = (ComboBox)Controls.Find("Debug", true)[0];
                if (combobox.SelectedIndex == 0)
                    Test_Method = 0;
                else if (combobox.SelectedIndex == 1)
                    Test_Method = 1;
                else
                    Test_Method = 0;

                if (Test_Method == 1)
                {
                    for (i = 0; i < 10; i++)
                    {
                        uitaskparameter[i].Pattern = 5;
                    }
                }

                combobox = (ComboBox)Controls.Find("CopyMethod", true)[0];
                if (combobox.SelectedIndex == 0)
                    Copy_Method = 0;
                else if (combobox.SelectedIndex == 1)
                    Copy_Method = 1;
                else
                    Copy_Method = 0;

            }


            public void CreateFile(byte Ti)
            {

                byte i = Ti;
                UInt32 file_num = 0;
                int file_num_total;
                UInt64 FileSize = 0;
                UInt64 One_Filesize = 0;
                Random ran = new Random();

                string strpath = uitaskparameter[i - 1].SourceDisk + ":\\" + "T" + i.ToString() + "_test";
                string CopyDestPath = strpath;
                CopyDestPath = CopyDestPath.Remove(0, 1);
                CopyDestPath = CopyDestPath.Insert(0, uitaskparameter[i - 1].DestDisk);
                //string strpath1 = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + i.ToString() + "_test";
                string path = CheckDir(strpath);

                //form2.output_T1.WriteLine("Create File...\n");
                WriteLogFile("from: " + strpath + " to: " + CopyDestPath + " with buffersize 16384. " + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);
                Lba[Ti - 1] = 0;



            if (uitaskparameter[i - 1].Selected == "enable")
            {
                switch (uitaskparameter[i - 1].FileSize)
                {
                    case 1:
                        {
                            file_num = 1;
                            while (file_num <= 100)
                            {
                                One_Filesize = ((uint)ran.Next() % (50 * 1024 * 1024 - 1024)) + 1024;//Min 1MB,Max 50MB
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                FileSize += One_Filesize;
                               // FileSize += FileSize + One_Filesize;
                                file_num++;
                            }
                        }
                        break;
                    case 2:
                        {
                            file_num = 1;
                            One_Filesize = (UInt32)2 * (UInt32)1024 * (UInt32)1024 * (UInt32)1024;
                            while (FileSize < (UInt64)10 * (UInt64)1024 * (UInt64)1024 * (UInt64)1024)
                            {
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num++;
                                FileSize += One_Filesize;
                            }
                        }
                        break;
                    case 3:
                        {
                            file_num = 1;
                            One_Filesize = 50 * 1024 * 1024;
                            if (Test_Method == 1)
                            {
                                datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                            }
                            Create_Pattern(One_Filesize, 1, i, path);
                        }
                        break;
                    case 4:
                        {
                            file_num = 1;
                            One_Filesize = 100 * 1024 * 1024;
                            if (Test_Method == 1)
                            {
                                datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                            }
                            Create_Pattern(One_Filesize, 1, i, path);   
                        }
                        break;
                    case 5:
                        {
                            file_num = 1;
                            One_Filesize = 500 * 1024 * 1024;
                            if (Test_Method == 1)
                            {
                                datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                            }
                            Create_Pattern(One_Filesize, 1, i, path);
                        }
                        break;
                    case 6:
                        {
                            file_num = 1;    
                            One_Filesize = 1024 * 1024 * 1024;
                            if (Test_Method == 1)
                            {
                                datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                            }
                            Create_Pattern(One_Filesize, 1, i, path);
                        }
                        break;
                    case 7:
                        {
                            file_num = 1;
                            One_Filesize = (UInt64)2 * (UInt64)1024 * (UInt64)1024 * (UInt64)1024;//2G
                            if (Test_Method == 1)
                            {
                                datacheck[i - 1, file_num - 1].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                datacheck[i - 1, file_num - 1].FileNumber = file_num.ToString();
                                datacheck[i - 1, file_num - 1].StartLba = Lba[i - 1];
                            }
                            Create_Pattern(One_Filesize, 1, i, path);
                        }
                        break;
                    case 8:
                        {
                            file_num_total = 0;
                            string path_basic = strpath ;
                            strpath = strpath + "\\200MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 3)
                            {
                                One_Filesize = 200 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\100MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 10)
                            {
                                One_Filesize = 100 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\50MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 15)
                            {
                                One_Filesize = 50 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\10MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 31)
                            {
                                One_Filesize = 10 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\5MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 50)
                            {
                                One_Filesize = 5 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\3MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 20)
                            {
                                One_Filesize = 3 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\1MB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 102)
                            {
                                One_Filesize = 1 * 1024 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                            strpath = path_basic;
                            strpath = strpath + "\\4KB";
                            path = CheckDir(strpath);
                            file_num = 1;
                            while (file_num <= 52)
                            {
                                One_Filesize = 4 * 1024;
                                if (Test_Method == 1)
                                {
                                    datacheck[i - 1, file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                                    datacheck[i - 1, file_num_total].FileNumber = file_num.ToString();
                                    datacheck[i - 1, file_num_total].StartLba = Lba[i - 1];
                                }
                                Create_Pattern(One_Filesize, file_num, i, path);
                                file_num_total++;
                                file_num++;
                            }
                        }
                        break;

                    }

                }
            }

            private void Create_Source_Pattern_Heavy_Loading(UInt64 FileSize,string Path,int FileNum)
            {
                UInt64[] DataContent_Debug = new UInt64[64];
                UInt64 FileSize_Display;
                int k;
                UInt64 j = 0;
                bool res;
                uint lenth = 1;

                FileSize_Display = FileSize / ((UInt32)1024 * (UInt32)1024);
                string hFile = Path + "\\test_" + FileNum.ToString() + "_" + FileSize_Display.ToString() + "MB.testtemp";
                File_src[0] = CreateFile(hFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                

                while (j < FileSize / 512)
                {
                    for (k = 0; k < 64; k++)
                    {
                        DataContent_Debug[k] = Lba[0];
                    }
                    Lba[0]++;
                    res = WriteFile(File_src[0], DataContent_Debug, 512, out lenth, 0);
                    j++;
                }
                res = CloseHandle(File_src[0]);
            }

            private void Create_Source_File_Heavy_Loading()
            {

                int i;
                string path_basic = "";
                for (i = 0; i < 10; i++)
                {
                    if (uitaskparameter[i].Selected == "enable")
                    {
                        path_basic = uitaskparameter[i].SourceDisk + ":\\S";
                        break;
                    }
                 }


                Delete_All_File(path_basic);

                //Thread.Sleep(5000);

                CheckDir(path_basic);
                
                string path;
                UInt64 One_Filesize;

                Lba[0] = 0;

                int file_num_total = 0;
                string strpath = path_basic;
                strpath = strpath + "\\200MB";
                path = CheckDir(strpath);
                int file_num = 1;
                while (file_num <= 3)
                {
                    One_Filesize = 200 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\100MB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 10)
                {
                    One_Filesize = 100 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\50MB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 15)
                {
                    One_Filesize = 50 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\10MB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 31)
                {
                    One_Filesize = 10 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\5MB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 50)
                {
                    One_Filesize = 5 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\3MB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 20)
                {
                    One_Filesize = 3 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\1MB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 102)
                {
                    One_Filesize = 1 * 1024 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }
                strpath = path_basic;
                strpath = strpath + "\\4KB";
                path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 52)
                {
                    One_Filesize = 4 * 1024;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024 * 1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading(One_Filesize, path, file_num);
                    file_num_total++;
                    file_num++;
                }

            }

            private void Create_Source_Pattern_Heavy_Loading_Special(UInt64 FileSize, string Path, int FileNum)
            {
                UInt64[] DataContent_Debug = new UInt64[64];
                UInt64 FileSize_Display;
                int k;
                UInt64 j = 0;
                bool res;
                uint lenth = 1;

                FileSize_Display = FileSize / ((UInt32)1024);
                string hFile = Path + "\\test_" + FileNum.ToString() + "_" + FileSize_Display.ToString() + "KB.testtemp";
                File_src[0] = CreateFile(hFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


                while (j < FileSize / 512)
                {
                    for (k = 0; k < 64; k++)
                    {
                        DataContent_Debug[k] = Lba[0];
                    }
                    Lba[0]++;
                    res = WriteFile(File_src[0], DataContent_Debug, 512, out lenth, 0);
                    j++;
                }
                if ((FileSize % 512) != 0)
                {
                    for (k = 0; k < 64; k++)
                    {
                        DataContent_Debug[k] = Lba[0];
                    }
                    Lba[0]++;
                    res = WriteFile(File_src[0], DataContent_Debug, (uint)(FileSize % 512), out lenth, 0);
                }
                res = CloseHandle(File_src[0]);
            }
            
            private void Create_Source_File_Heavy_Loading_Special()
            {

                int i;
                string path_basic = "";
                for (i = 0; i < 10; i++)
                {
                    if (uitaskparameter[i].Selected == "enable")
                    {
                        path_basic = uitaskparameter[i].SourceDisk + ":\\S";
                        break;
                    }
                }


                Delete_All_File(path_basic);

                //Thread.Sleep(5000);

                CheckDir(path_basic);

                string path;
                UInt64 One_Filesize;

                Lba[0] = 0;

                int file_num_total = 0;
                string strpath = path_basic;
                //strpath = strpath + "\\200MB";
                path = CheckDir(strpath);
                int file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 7305;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\100MB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 7302;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\50MB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 7143;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\10MB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 61142;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\5MB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 41785;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\3MB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 206;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\1MB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 987029;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                //strpath = path_basic;
                //strpath = strpath + "\\4KB";
                //path = CheckDir(strpath);
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 733776;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 3235;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 3215;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 67584;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 2790;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 7311;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 1)
                {
                    One_Filesize = 15360;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 2)
                {
                    One_Filesize = 6054400;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 2)
                {
                    One_Filesize = 887296;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }
                file_num = 1;
                while (file_num <= 2)
                {
                    One_Filesize = 1081344;
                    if (Test_Method == 1)
                    {
                        datacheck_HL[file_num_total].FileSize = (One_Filesize / (1024)).ToString();
                        datacheck_HL[file_num_total].FileNumber = file_num_total.ToString();
                        datacheck_HL[file_num_total].StartLba = Lba[0];
                    }
                    Create_Source_Pattern_Heavy_Loading_Special(One_Filesize, path, file_num_total);
                    file_num_total++;
                    file_num++;
                }

            }

            public UInt64[] Lba = new UInt64[10];

         
            private void Create_Pattern(UInt64 FileSize, UInt32 file_num, byte Ti, string Path)
            {
                byte i = Ti;
                ulong j = 0;
                UInt64 FileSize_Display;
                Random ra = new Random();
                Byte[] DataContent = new Byte[1024];
                UInt64[] DataContent_Debug = new UInt64[64];
                
                uint n;
                uint lenth = 1;
                Byte Ascending = 0;
                bool res;
                uint k;

                FileSize_Display = FileSize / ((UInt32)1024 * (UInt32)1024);
                string hFile = Path + "\\test_" + file_num.ToString() + "_" + FileSize_Display.ToString() + "MB.testtemp";
                File_src[i - 1] = CreateFile(hFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


                switch (uitaskparameter[i - 1].Pattern)
                {
                    
                    case 1:
                        {
                            while (j < FileSize / 1024)
                            {
                                for (k = 0; k < 1024; k++)
                                {
                                    n = (uint)ra.Next() * (uint)ra.Next();
                                    DataContent[k] = (Byte)n;
                                }
                                //DataContent.CopyTo(DataContent_Debug, 0);
                                //DataContent_Debug = DataContent.Clone();
                                //Array.Copy(DataContent, DataContent_Debug, 1024);
                                //Buffer.BlockCopy(DataContent, 0, DataContent_Debug, 0, 1024);

                                for (k = 0; k < 64; k++)
                                {
                                    DataContent_Debug[k] = BitConverter.ToUInt64(DataContent, (int)k * 8);
                                }
                                    res = WriteFile(File_src[i - 1], DataContent_Debug, 1024, out lenth, 0);
                                j++;
                            }
                            res = CloseHandle(File_src[i - 1]);
                        }
                        break;
                    case 2:
                        {
                            while (j < FileSize / 512)
                            {
                                for (k = 0; k < 1024; k++)
                                    DataContent[k] = 0xAA;
                                for (k = 0; k < 64; k++)
                                {
                                    DataContent_Debug[k] = BitConverter.ToUInt64(DataContent, (int)k * 8);
                                }
                                res = WriteFile(File_src[i - 1], DataContent_Debug, 512, out lenth, 0);
                                j++;
                            }
                            res = CloseHandle(File_src[i - 1]);
                        }
                        break;
                    case 3:
                        {
                            while (j < FileSize / 512)
                            {
                                for (k = 0; k < 1024; k++)
                                    DataContent[k] = 0x55;
                                for (k = 0; k < 64; k++)
                                {
                                    DataContent_Debug[k] = BitConverter.ToUInt64(DataContent, (int)k * 8);
                                }
                                res = WriteFile(File_src[i - 1], DataContent_Debug, 512, out lenth, 0);
                                j++;
                            }
                            res = CloseHandle(File_src[i - 1]);
                        }
                        break;
                    case 4:
                        {
                            while (j < FileSize / 512)
                            {
                                for (k = 0; k < 1024; k++)
                                {
                                    DataContent[k] = Ascending;
                                    Ascending++;
                                    if (Ascending > 0xFF)
                                        Ascending = 0;
                                }
                                for (k = 0; k < 64; k++)
                                {
                                    DataContent_Debug[k] = BitConverter.ToUInt64(DataContent, (int)k * 8);
                                }
                                res = WriteFile(File_src[i - 1], DataContent_Debug, 512, out lenth, 0);
                                j++;
                            }
                            res = CloseHandle(File_src[i - 1]);
                        }
                        break;
                    case 5:
                        {
                            while (j < FileSize / 512)
                            {
                                for (k = 0; k < 64; k++)
                                {
                                    DataContent_Debug[k] = Lba[Ti - 1];
                                }
                                Lba[Ti - 1]++;
                                res = WriteFile(File_src[i - 1], DataContent_Debug, 512, out lenth, 0);
                                j++;
                            }
                            res = CloseHandle(File_src[i - 1]);
                        }
                        break;
                }

            }

            private double ComputeTime(DateTime TimeStart, DateTime TimeEnd)
            {
                int Hour, Minute, Second, Millisecond;
                double Result;

                Hour = TimeEnd.Hour - TimeStart.Hour;
                if (Hour < 0)
                    Hour += 24;
                Minute = TimeEnd.Minute - TimeStart.Minute;
                if (Minute < 0)
                {
                    Minute += 60;
                    Hour -= 1;
                }
                Second = TimeEnd.Second - TimeStart.Second;
                if (Second < 0)
                {
                    Second += 60;
                    Minute -= 1;
                }
                Millisecond = TimeEnd.Millisecond - TimeStart.Millisecond;
                if (Millisecond < 0)
                {
                    Millisecond += 1000;
                    Second -= 1;
                }
                Result = (double)Hour * (double)60 * (double)60 + (double)Minute * (double)60 + (double)Second + (double)Millisecond / (double)1000;
                return (Result);

            }
            

            private void Copy_One_File(string Src_Path, string Dest_Path, byte Ti)
            {
                bool res, des;
                const int BUFFERSIZE = 2 * 1024;
                UInt64[] Buffer_Src = new UInt64[BUFFERSIZE];
                UInt64[] Buffer_Des = new UInt64[BUFFERSIZE];
                int i, j;
                int loop = 0;

                double result;
                DateTime TimeStart = new DateTime(), TimeEnd = new DateTime();
                int offsetS = 0, offsetE = 0;
                
                uint Lenth_src, Lenth_dest;
              
                //int debug = 0;

                UInt64 Lba_check = 0,Lba_current = 0;

                
                res = des = false;
                while (!res)
                {
                    File_src[Ti - 1] = CreateFile(Src_Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                    if (File_src[Ti - 1] == INVALID_HANDLE_VALUE)
                    {
                        WriteLogFile("Src path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        res = false;
                    }
                    else
                    {
                        res = true;
                    }
                }

                while (!des)
                {
                    File_dest[Ti - 1] = CreateFile(Dest_Path, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                    if (File_dest[Ti - 1] == INVALID_HANDLE_VALUE)
                    {
                        WriteLogFile("Src path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        des = false;
                    }
                    else
                    {
                        des = true;
                    }
                }

                Lenth_dest = Lenth_src = 1;
                if (Test_Function == 0)
                {
                    Lba_check = datacheck[Ti - 1, checkresult[Ti - 1]].StartLba;
                }
                else
                {
                    Lba_check = datacheck_HL[checkresult[Ti - 1]].StartLba;
                }

                while (true)
                {
                    if (Test_Function == 0)
                    {
                        TimeStart = DateTime.Now;//recordtime
                    }
                    res = des = false;
                    while (!res)
                    {
                        res = ReadFile(File_src[Ti - 1], Buffer_Src, 16 * 1024, out Lenth_src, 0);
                    }
                    /*if (loop == 63)
                    {
                        debug = 1;
                    }*/
                    if (Test_Method == 1)
                    {
                        Lba_current = Lba_check;
                        j = 0;
                        for (i = 0; i < Lenth_src / 8; i++)
                        {

                            /* if (debug == 1)
                             {
                                 if (i == 0x7fe)
                                     debug = 1;
                             }*/
                            if (Buffer_Src[i] != Lba_check)
                            {
                                WriteLogFile("Copy read src error! Error Src file is " + Src_Path + ". Want to write file is " + Dest_Path + ". Error Lba is " + Buffer_Src[i].ToString() + ". Correct Lba is " + Lba_check.ToString() + ".\t\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                Stop_Test_HL(Ti);
                            }
                            else
                            {
                                if (j < 63)
                                {
                                    j++;
                                }
                                else
                                {
                                    Lba_check++;
                                    j = 0;
                                }
                            }
                        }
                    }

                    if (Lenth_src != 16384)
                    {
                        while (!des)
                        {
                            des = WriteFile(File_dest[Ti - 1], Buffer_Src, Lenth_src, out Lenth_dest, 0);
                        }
                        break;
                    }
                    while (!des)
                    {
                        des = WriteFile(File_dest[Ti - 1], Buffer_Src, 16 * 1024, out Lenth_dest, 0);
                    }
                    if (Test_Function == 0)
                    {
                        TimeEnd = DateTime.Now;//recordtime
                        result = ComputeTime(TimeStart, TimeEnd);
                        if (result > Convert.ToDouble(uitaskparameter[Ti - 1].Time))
                        {
                            offsetS = loop * 16 * 1024;
                            offsetE = (loop + 1) * 16 * 1024;
                            WriteLogFile("Timeout Error: write took > " + uitaskparameter[Ti - 1].Time + "s at Copy file." + " (" + result.ToString() + ")  File: " + Dest_Path + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                            WriteLogFile("Offset: " + offsetS.ToString() + " - " + offsetE.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);//
                  
                            //continue;
                        }
                    }
                    if (Test_Method == 1)
                    {
                        SetFilePointer(File_dest[Ti - 1], (uint)(loop * 16 * 1024), (uint)0, FILE_BEGIN);
                        res = false;
                        while (!res)
                        {
                            res = ReadFile(File_dest[Ti - 1], Buffer_Des, 16 * 1024, out Lenth_src, 0);
                        }
                        j = 0;
                        for (i = 0; i < Lenth_src / 8; i++)
                        {

                            if (Buffer_Src[i] != Lba_current)
                            {
                                WriteLogFile("Copy Write dest error! Error Dest file is " + Dest_Path + ". Src file is " + Src_Path + ". Error Lba is " + Buffer_Src[i].ToString() + ". Correct Lba is " + Lba_current.ToString() + ".\t\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                Stop_Test_HL(Ti);
                            }
                            else
                            {
                                if (j < 63)
                                {
                                    j++;
                                }
                                else
                                {
                                    Lba_current++;
                                    j = 0;
                                }
                            }
                        }
                    }
                    loop++;
                    
                   
                }
                if (Test_Method == 1)
                {
                    SetFilePointer(File_dest[Ti - 1], (uint)(loop * 16 * 1024), (uint)0, FILE_BEGIN);
                    res = false;
                    while (!res)
                    {
                        res = ReadFile(File_dest[Ti - 1], Buffer_Des, Lenth_src, out Lenth_dest, 0);
                    }
                    j = 0;
                    for (i = 0; i < Lenth_dest / 8; i++)
                    {

                        if (Buffer_Src[i] != Lba_current)
                        {
                            WriteLogFile("Copy Write dest error! Error file is " + Dest_Path + ". Error Lba is " + Buffer_Src[i].ToString() + ". Correct Lba is " + Lba_current.ToString() + ".\t\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        }
                        else
                        {
                            if (j < 63)
                            {
                                j++;
                            }
                            else
                            {
                                Lba_current++;
                                j = 0;
                            }
                        }
                    }
                }

                CloseHandle(File_dest[Ti - 1]);
                CloseHandle(File_src[Ti - 1]);


            }

            private void Copy_File_Multi_Task(byte Ti)
            {
                string FilePath_Des, FilePath_Src;

                FilePath_Src = uitaskparameter[Ti - 1].SourceDisk + ":\\" + "T" + Ti.ToString() + "_test";
                FilePath_Des = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test";

                Copy_File_Multi_Path(FilePath_Src, FilePath_Des, Ti);
            }

         

            private int Find_Data_Check_Table(string file_size, string file_num,byte Ti)
            {
                int i = 0;

                while (i < 300)
                {
                    if (Test_Function == 0)
                    {
                        if (string.Equals(datacheck[Ti - 1, i].FileSize + "KB", file_size) && string.Equals(datacheck[Ti - 1, i].FileNumber, file_num))
                        {
                            return i;
                            //break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    else
                    {
                        if (string.Equals(datacheck_HL[i].FileSize + "KB", file_size) && string.Equals(datacheck_HL[i].FileNumber, file_num))
                        {
                            return i;
                            //break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                }
                return i;
            }

          
            public int[] checkresult = new int[10];
            private void Copy_File_Multi_Path(string Path_Source, string Path_Destination,Byte Ti)
            {
                 
                string Src_Path, Dest_Path;
                string filenum;
                string filesize;
                bool res = false;

                WIN32_FIND_DATA FindFileData = new WIN32_FIND_DATA();
                

                System.IntPtr hFind_Copy = INVALID_HANDLE_VALUE;
                System.IntPtr hCopy = INVALID_HANDLE_VALUE;


                // IntPtr hFind_Copy;

                 Src_Path = Path_Source + "\\*.*"; 
                
                 hFind_Copy = FindFirstFile(Src_Path, ref FindFileData);

                 if (hFind_Copy == INVALID_HANDLE_VALUE)
                 {
                     WriteLogFile("Pattern path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                     //getch();
                 }

                 //sFormatFileName = ".\\output\\Copy";
                 Dest_Path = Path_Destination;
                 CheckDir(Dest_Path);


                 while (FindNextFile(hFind_Copy, ref FindFileData))
                 {
                     if (FindFileData.cFileName.Equals(@".") || FindFileData.cFileName.Equals(@".."))
                     {
                         continue;
                     }

                     if ((FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes) != 0)
                     {

                         Dest_Path = Path_Destination + "\\" + FindFileData.cFileName;
                         Src_Path = Path_Source + "\\" + FindFileData.cFileName;
                         
                         Copy_File_Multi_Path(Src_Path, Dest_Path,Ti);
                         //FindNextFile(hFind_Copy, &FindFileData);
                     }
                     else
                     {
                         Src_Path = Path_Source + "\\" + FindFileData.cFileName;
                         Dest_Path = Path_Destination + "\\" + FindFileData.cFileName;
                         WriteLogFile("Copy File from " + Src_Path + " to " + Dest_Path + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

                         /*string FileName = FindFileData.cFileName;
                         char[] seperator = {'_','_','.'};
                         string[] slipstrings = new string[100];
                         slipstrings = FileName.Split(seperator);
                         filenum = slipstrings[1];
                         filesize = slipstrings[2];
                         checkresult[Ti-1] = Find_Data_Check(filesize, filenum);*/

                         if (Test_Function == 1)
                         {
                             if (Test_Method == 0)
                             {
                                 if (Copy_Method == 0)
                                 {
                                     res = false;
                                     while (!res)
                                     {
                                         hCopy = CopyFileA(Src_Path, Dest_Path, false);
                                         if (hCopy == INVALID_HANDLE_VALUE)
                                         {
                                             res = false;
                                             WriteLogFile("Copy File Error.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                         }
                                         else
                                         {
                                             res = true;
                                         }
                                     }
                                 }
                                 else
                                 {
                                     Copy_One_File(Src_Path, Dest_Path, Ti);
                                 }
                             }
                             else
                             {
                                 string FileName = FindFileData.cFileName;
                                 char[] seperator = { '_', '_', '.' };
                                 string[] slipstrings = new string[100];
                                 slipstrings = FileName.Split(seperator);
                                 filenum = slipstrings[1];
                                 filesize = slipstrings[2];
                                 //checkresult[Ti - 1] = Find_Data_Check(filesize, filenum);
                                 checkresult[Ti - 1] = Find_Data_Check_Table(filesize, filenum, Ti);
                                 Copy_One_File(Src_Path, Dest_Path, Ti);
                             }
                         }
                         else
                         {
                             if (Test_Method == 1)
                             {
                                 string FileName = FindFileData.cFileName;
                                 char[] seperator = { '_', '_', '.' };
                                 string[] slipstrings = new string[100];
                                 slipstrings = FileName.Split(seperator);
                                 filenum = slipstrings[1];
                                 filesize = slipstrings[2];
                                 //checkresult[Ti - 1] = Find_Data_Check(filesize, filenum);
                                 checkresult[Ti - 1] = Find_Data_Check_Table(filesize, filenum,Ti);

                                 Copy_One_File(Src_Path, Dest_Path, Ti);
                             }
                             else
                             {
                                 Copy_One_File(Src_Path, Dest_Path, Ti);
                             }
                         }

                         /*res = false;
                         while (!res)
                         {
                             hCopy = CopyFileA(Src_Path, Dest_Path, false);
                             if (hCopy == INVALID_HANDLE_VALUE)
                             {
                                 res = false;
                                 WriteLogFile("Copy File Error.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                             }
                             else
                             {
                                 res = true;
                             }
                         }*/

                         //Copy_One_File(Src_Path, Dest_Path, Ti);


                     }

                 }
 



            }

            private int Compute_Copy_Number()
            {
                int Thread_Num = 0,Copy_Num;
                ulong size_available, size_total, size_free;
                int count = 0;

                while (count < 10)
                {
                    if (uitaskparameter[count].Selected == "enable")
                        Thread_Num++;
                    count++;
                }

                GetDiskFreeSpaceEx(uitaskparameter[0].DestDisk + ":\\", out size_available, out size_total, out size_free);
                Copy_Num = (int)(size_available / ((ulong)18 * (ulong) 1024 * (ulong) 1024 * (ulong) Thread_Num));
                return (Copy_Num);


            }

            private void Copy_File_HeavyLoading(byte Ti,int Copy_Num_true)
            {
                

                //IntPtr File_src, File_dest;

                
                //int j = 1, k = 1, offsetS = 0, offsetE = 0;

                string FilePath_Des, FilePath_Src;
                //char[] Arry_path1, Arry_path2;
                

                //bool res, des;


                

                if (Copy_Num_true == 0)
                {

                    FilePath_Src = uitaskparameter[Ti - 1].SourceDisk + ":\\" + "S";


                    FilePath_Des = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test\\Copy_" + Copy_Num_true.ToString();

                }
                else
                {
                    FilePath_Src = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test\\Copy_" + (Copy_Num_true - 1).ToString();


                    FilePath_Des = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test\\Copy_" + Copy_Num_true.ToString();

                }
                Copy_File_Multi_Path(FilePath_Src, FilePath_Des, Ti);
               

            }

          

            public void Stop_Test_HL(Byte Ti)
            {

                int i;

                for (i = 0; i < 10; i++)
                {
                    if (uitaskparameter[i].Selected == "enable")
                    {
                        if (i == (Ti - 1))
                            continue;
                        else
                        {
                            CloseHandle(File_dest[i]);
                            CloseHandle(File_src[i]);
                            Task[i].Abort();
                        }
                    }
                }
                Task[Ti - 1].Abort();

            }


            public void Stop_Test_MT(Byte Ti)
            {

                int i;

                for (i = 0; i < 10; i++)
                {
                    if (uitaskparameter[i].Selected == "enable")
                    {
                        if (i == (Ti - 1))
                            continue;
                        else
                        {
                            CloseHandle(File_dest[i]);
                            CloseHandle(File_src[i]);
                            Task[i].Abort();
                        }
                    }
                }
                Task[Ti - 1].Abort();

                Display_Red_Color_For_Error_Task(Ti);

            }

            private void Write_Buffer_To_File(byte[] Buffer_src, byte[] Buffer_des)
            {


                string strpath;
                
                string DirePath;
                
                strpath = "../";
                DirePath = CheckDir(strpath);     //得到上层目录
                strpath = DirePath + "\\output";
                DirePath = CheckDir(strpath);     //在上层目录中建一个output文件夹

                strpath = DirePath + "\\error_src.bin";
                CreateFile(strpath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


                FileStream MyFile = new FileStream(strpath, FileMode.Open, FileAccess.Write, FileShare.ReadWrite);
                //buffer = System.Text.Encoding.Default.GetBytes(msg);
                MyFile.Write(Buffer_src, 0, Buffer_src.GetLength(0));
                MyFile.Close();

                strpath = DirePath + "\\error_des.bin";
                CreateFile(strpath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

                MyFile = new FileStream(strpath, FileMode.Open, FileAccess.Write, FileShare.ReadWrite);
                MyFile.Write(Buffer_des, 0, Buffer_des.GetLength(0));
                MyFile.Close();


            }
            private void Write_Buffer_To_File_U64(UInt64[] Buffer_src, UInt64[] Buffer_des)
            {


                string strpath;

                string DirePath;

                IntPtr FileHandle;
                uint length_src = 1;

                strpath = "../";
                DirePath = CheckDir(strpath);     //得到上层目录
                strpath = DirePath + "\\output";
                DirePath = CheckDir(strpath);     //在上层目录中建一个output文件夹

                strpath = DirePath + "\\error_src.bin";
                FileHandle = CreateFile(strpath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

                WriteFile(FileHandle, Buffer_src, 16 * 1024, out length_src, 0);

                
                strpath = DirePath + "\\error_des.bin";
                FileHandle = CreateFile(strpath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

                WriteFile(FileHandle, Buffer_des, 16 * 1024, out length_src, 0);

                CloseHandle(FileHandle);
                


            }

            private int BufferEquals(byte[] b1, byte[] b2, UInt32 length)
            {
                if (b1.Length != b2.Length) return 17000;
                if (b1 == null || b2 == null) return 17001;
                for (int i = 0; i < length; i++)
                    if (b1[i] != b2[i])
                        return i;
                return 18000;
            }

            private int BufferEquals_U64(UInt64[] b1, UInt64[] b2, UInt32 length)
            {
                if (b1.Length != b2.Length) return 17000;
                if (b1 == null || b2 == null) return 17001;
                for (int i = 0; i < length/8; i++)
                    if (b1[i] != b2[i])
                        return i;
                return 18000;
            }

            private int BufferEquals_1(byte[] b1, byte[] b2, UInt32 length)
            {
                if (b1.Length != b2.Length) return 17000;
                if (b1 == null || b2 == null) return 17001;
                for (int i = 0; i < length; i++)
                    if (b1[i] != 0xAA || b2[i] != 0xAA)
                        return i;
                return 18000;
            }

            private void Compare_File_Multi_Task(byte Ti)
            {
                string FilePath_Src, FilePath_Des;


                FilePath_Src = uitaskparameter[Ti - 1].SourceDisk + ":\\" + "T" + Ti.ToString() + "_test";
                FilePath_Des = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test";

                Compare_File_Multi_Path(FilePath_Src, FilePath_Des, Ti);

            }

          

            private void Display_Red_Color_For_Error_Task(byte Ti)
            {
                switch (Ti)
                {

                    case 1:
                        {
                            this.form2.BackColor = Color.Red;
                        }
                        break;
                    case 2:
                        {
                            this.form3.BackColor = Color.Red;
                        }
                        break;
                    case 3:
                        {
                            this.form4.BackColor = Color.Red;
                        }
                        break;
                    case 4:
                        {
                            this.form5.BackColor = Color.Red;
                        }
                        break;
                    case 5:
                        {
                            this.form6.BackColor = Color.Red;
                        }
                        break;
                    case 6:
                        {
                            this.form7.BackColor = Color.Red;
                        }
                        break;
                    case 7:
                        {
                            this.form8.BackColor = Color.Red;
                        }
                        break;
                    case 8:
                        {
                            this.form9.BackColor = Color.Red;
                        }
                        break;
                    case 9:
                        {
                            this.form10.BackColor = Color.Red;
                        }
                        break;
                    case 10:
                        {
                            this.form11.BackColor = Color.Red;
                        }
                        break;
                }

            }

            private void Compare_File_Multi_Path(string Path_Source, string Path_Destination,Byte Ti)
            {
                string Src_Path, Dest_Path;
                int j = 1, k = 0, offset = 0;

                WIN32_FIND_DATA FindFileData = new WIN32_FIND_DATA();

                uint Lenth_src, Lenth_dest;
                bool res, des;
                //FileStream fs;

                const int BUFFERSIZE = 2 * 1024;
                UInt64[] Buffer_src = new UInt64[BUFFERSIZE];
                UInt64[] Buffer_dest = new UInt64[BUFFERSIZE];
                int Compare_result;
                //Path_Source = uitaskparameter[Ti - 1].SourceDisk + ":\\S";


                System.IntPtr hFind_Copy = INVALID_HANDLE_VALUE;


                // IntPtr hFind_Copy;

                Src_Path = Path_Source + "\\*.*";

                hFind_Copy = FindFirstFile(Src_Path, ref FindFileData);

                if (hFind_Copy == INVALID_HANDLE_VALUE)
                {
                    WriteLogFile("Pattern path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                    //getch();
                }

                //sFormatFileName = ".\\output\\Copy";
                //Path_Destination = uitaskparameter[Ti - 1].DestDisk + ":\\T" + Ti.ToString() + "_test\\Copy_" + Copy_Num.ToString();
                //CheckDir(Dest_Path);


                while (FindNextFile(hFind_Copy, ref FindFileData))
                {
                    if (FindFileData.cFileName.Equals(@".") || FindFileData.cFileName.Equals(@".."))
                    {
                        continue;
                    }

                    if ((FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes) != 0)
                    {

                        Dest_Path = Path_Destination + "\\" + FindFileData.cFileName;
                        Src_Path = Path_Source + "\\" + FindFileData.cFileName;

                        Compare_File_Multi_Path(Src_Path, Dest_Path, Ti);
                        //FindNextFile(hFind_Copy, &FindFileData);
                    }
                    else
                    {
                        Src_Path = Path_Source + "\\" + FindFileData.cFileName;
                        Dest_Path = Path_Destination + "\\" + FindFileData.cFileName;
                        WriteLogFile("Compare File from " + Src_Path + " to " + Dest_Path + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        //CopyFileA(Src_Path, Dest_Path, false);
                        k = 0;
                        res = des = false;
                        while (!des)
                        {
                            File_dest[Ti - 1] = CreateFile(Dest_Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                            if (File_dest[Ti - 1] == INVALID_HANDLE_VALUE)
                            {
                                WriteLogFile("Dest path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                des = false;
                            }
                            else
                            {
                                des = true;
                            }
                        }
                        while (!res)
                        {
                            File_src[Ti - 1] = CreateFile(Src_Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                            if (File_src[Ti - 1] == INVALID_HANDLE_VALUE)
                            {
                                WriteLogFile("Src path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                res = false;
                            }
                            else
                            {
                                res = true;
                            }

                        }

                        Lenth_dest = Lenth_src = 1;

                        while (true)
                        {

                            res = des = false;
                            while (!res)
                            {
                                res = ReadFile(File_src[Ti - 1], Buffer_src, 16 * 1024, out Lenth_src, 0);
                                
                            }
                           
                            while (!des)
                            {
                                des = ReadFile(File_dest[Ti - 1], Buffer_dest, 16 * 1024, out Lenth_dest, 0);
                            }
                            
                            if (Lenth_src != 16384)
                            {
                                Compare_result = BufferEquals_U64(Buffer_src, Buffer_dest, Lenth_src);
                                if (Compare_result == 18000)
                                    break;
                                else if (Compare_result == 17000)
                                {
                                    
                                    WriteLogFile("Compare Error: No." + j.ToString() + " buffer size is not equaled.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    j++;
                                    break;
                                }
                                else if (Compare_result == 17001)
                                {
                                    
                                    WriteLogFile("Compare Error: No." + j.ToString() + " buffer_src or buffer_dest  is null.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    j++;
                                    break;
                                }
                                else
                                {
                                    offset = k * 16 * 1024 + (Compare_result * 8);
                                    WriteLogFile("Compare error des file: " + Dest_Path  + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

                                    WriteLogFile("Compare Error: No." + j.ToString() + "buffer_src content is " + Buffer_src[Compare_result].ToString() + ".buffer_dest content is " + Buffer_dest[Compare_result].ToString() + ". offset is " + offset.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    //CloseHandle(File_dest[Ti - 1]);
                                    SetFilePointer(File_dest[Ti - 1], (uint)(k * 16 * 1024), (uint)0, FILE_BEGIN);
                                    ReadFile(File_dest[Ti - 1], Buffer_dest, 16 * 1024, out Lenth_dest, 0);
                                    /*fs = File.OpenRead(Dest_Path);
                                    fs.Seek(k * 16 * 1024, SeekOrigin.Begin);
                                    fs.Read(Buffer_dest, (int)0, (int)Lenth_src);*/
                                    Compare_result = BufferEquals_U64(Buffer_src, Buffer_dest, Lenth_src);
                                    if (Compare_result == 18000)
                                    {
                                        WriteLogFile("Compare Result Correct when read dest file again!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                        Display_Red_Color_For_Error_Task(Ti);
                                        Stop_Test_HL(Ti);
                                    }
                                    else if (Compare_result == 17000)
                                    {
                                        //Console.WriteLine("buffer size is not equaled.");
                                        WriteLogFile("Read dest file again. Compare Error: No." + j.ToString() + " buffer size is not equaled.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                        Display_Red_Color_For_Error_Task(Ti);
                                        Stop_Test_HL(Ti);
                                    }
                                    else if (Compare_result == 17001)
                                    {
                                        //Console.WriteLine("buffer_src or buffer_dest  is null.");
                                        WriteLogFile("Read dest file again. Compare Error: No." + j.ToString() + " buffer_src or buffer_dest  is null.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                        Display_Red_Color_For_Error_Task(Ti);
                                        Stop_Test_HL(Ti);
                                    }
                                    else
                                    {
                                        WriteLogFile("Read dest file again.Compare Error: No." + j.ToString() + "buffer_src content is " + Buffer_src[Compare_result].ToString() + ".buffer_dest content is " + Buffer_dest[Compare_result].ToString() + ". offset is " + offset.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                        Write_Buffer_To_File_U64(Buffer_src, Buffer_dest);
                                        Display_Red_Color_For_Error_Task(Ti);
                                        Stop_Test_HL(Ti);
                                    }
                                    j++;
                                    break;
                                }
                            }
                            Compare_result = BufferEquals_U64(Buffer_src, Buffer_dest, (uint)Buffer_src.Length);

                            if (Compare_result == 18000)
                                ;
                            else if (Compare_result == 17000)
                            {
                                //Console.WriteLine(" buffer size is not equaled.");
                                WriteLogFile("Compare Error: No." + j.ToString() + " buffer size is not equaled.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                Display_Red_Color_For_Error_Task(Ti);
                                Stop_Test_HL(Ti);
                                j++;
                                break;
                            }
                            else if (Compare_result == 17001)
                            {
                                //Console.WriteLine("buffer_src or buffer_dest  is null.");
                                WriteLogFile("Compare Error: No." + j.ToString() + " buffer_src or buffer_dest  is null.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                Display_Red_Color_For_Error_Task(Ti);
                                Stop_Test_HL(Ti);
                                j++;
                                break;
                            }
                            else
                            {
                                offset = k * 16 * 1024 + (Compare_result * 8); ;
                                //Console.WriteLine("buffer_src content is {0}.buffer_dest content is {1}. offset is {2}.", Buffer_src[Compare_result], Buffer_dest[Compare_result], Compare_result);
                                //CloseHandle(File_dest[Ti - 1]);
                                WriteLogFile("Compare error des file: " + Dest_Path + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

                                WriteLogFile("Compare Error: No." + j.ToString() + "buffer_src content is " + Buffer_src[Compare_result].ToString() + ".buffer_dest content is " + Buffer_dest[Compare_result].ToString() + ". offset is " + offset.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                SetFilePointer(File_dest[Ti - 1], (uint)(k * 16 * 1024), (uint)0, FILE_BEGIN);
                                ReadFile(File_dest[Ti - 1], Buffer_dest, 16 * 1024, out Lenth_dest, 0);
                                
                                /*fs = File.OpenRead(Dest_Path);
                                fs.Seek(k * 16 * 1024, SeekOrigin.Begin);
                                fs.Read(Buffer_dest, 0, (int)Buffer_dest.Length);*/
                                Compare_result = BufferEquals_U64(Buffer_src, Buffer_dest, (uint)Lenth_dest);
                                if (Compare_result == 18000)
                                {
                                    WriteLogFile("Compare Result Correct when read dest file again!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    Display_Red_Color_For_Error_Task(Ti);

                                    Stop_Test_HL(Ti);
                                }
                                else if (Compare_result == 17000)
                                {
                                    //Console.WriteLine("buffer size is not equaled.");
                                    WriteLogFile("Read dest file again. Compare Error: No." + j.ToString() + " buffer size is not equaled.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    j++;
                                    Display_Red_Color_For_Error_Task(Ti);
                                    Stop_Test_HL(Ti);
                                }
                                else if (Compare_result == 17001)
                                {
                                    //Console.WriteLine("buffer_src or buffer_dest  is null.");
                                    WriteLogFile("Read dest file again. Compare Error: No." + j.ToString() + " buffer_src or buffer_dest  is null.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    j++;
                                    Display_Red_Color_For_Error_Task(Ti);
                                    Stop_Test_HL(Ti);
                                }
                                else
                                {
                                    WriteLogFile("Read dest file again.Compare Error: No." + j.ToString() + "buffer_src content is " + Buffer_src[Compare_result].ToString() + ".buffer_dest content is " + Buffer_dest[Compare_result].ToString() + ". offset is " + offset.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                                    Write_Buffer_To_File_U64(Buffer_src, Buffer_dest);
                                    Display_Red_Color_For_Error_Task(Ti);
                                    Stop_Test_HL(Ti);
                                }
                                j++;
                                break;
                            }
                            k++;
                        }
                        CloseHandle(File_dest[Ti - 1]);
                        CloseHandle(File_src[Ti - 1]);


                    }

                }
    
            }

            private void Compare_File_Heavy_Loading(byte Ti, int Copy_Num)
            {

                string FilePath_Src, FilePath_Dest;
                //Stop_Test_HL();

                FilePath_Src = uitaskparameter[Ti - 1].SourceDisk + ":\\S";
                FilePath_Dest = uitaskparameter[Ti - 1].DestDisk + ":\\T" + Ti.ToString() + "_test\\Copy_" + Copy_Num.ToString();

                Compare_File_Multi_Path(FilePath_Src, FilePath_Dest, Ti);

                
                
                
            }

            private void Unlock_All_File(string Path)
            {
                
                string Dest_Path, Cmd; /*Pid, ProcessPath;*/
                StreamWriter fs;
                
                /*Process CurrentProcess = Process.GetCurrentProcess();
                Pid = CurrentProcess.Id.ToString();//PID
                ProcessPath = CurrentProcess.MainModule.FileName;*/
                
                

                WIN32_FIND_DATA FindFileData = new WIN32_FIND_DATA();

                //WIN32_FIND_DATA FindFileData;


                System.IntPtr hFind_Copy = INVALID_HANDLE_VALUE;


                // IntPtr hFind_Copy;

                Dest_Path = Path + "\\*.*";

                hFind_Copy = FindFirstFile(Dest_Path, ref FindFileData);

                if (hFind_Copy == INVALID_HANDLE_VALUE)
                {
                    //WriteLogFile("Pattern path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                    //getch();
                }

                //sFormatFileName = ".\\output\\Copy";

                Cmd = "call .\\unlocker.exe  " + Path + " /S";

                fs = new StreamWriter(".\\unlock.bat");
                fs.WriteLine(Cmd);
                fs.Close();

                System.Diagnostics.Process.Start(".\\unlock.bat");
                Thread.Sleep(500);


                while (FindNextFile(hFind_Copy, ref FindFileData))
                {
                    if (FindFileData.cFileName.Equals(@".") || FindFileData.cFileName.Equals(@".."))
                    {
                        continue;
                    }

                    if ((FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes) != 0)
                    {

                        Dest_Path = Path + "\\" + FindFileData.cFileName;
                        //Cmd = "call C:\\Users\\anvilawang\\Desktop\\1114\\newMultiTaskCsharp_UI\\MultiTaskCsharp\\bin\\Release\\unlocker.exe  " + Dest_Path + " /S";
                        /*Cmd = "call .\\unlocker.exe  " + Dest_Path + " /S";

                        fs = new StreamWriter(".\\unlock.bat");
                        fs.WriteLine(Cmd);
                        fs.Close();

                        System.Diagnostics.Process.Start(".\\unlock.bat");
                        Thread.Sleep(500);*/
                        Unlock_All_File(Dest_Path);
                        //RemoveDirectory(Dest_Path);
                        //System.IO.Directory.Delete(Path_dir, true);
                        //FindNextFile(hFind_Copy, &FindFileData);
                    }
                    else
                    {
                        Dest_Path = Path + "\\" + FindFileData.cFileName;
                        //WriteLogFile("Delete File " + Dest_Path + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        //System.IO.File.Delete(Dest_Path);

                    }

                }

                CloseHandle(hFind_Copy);

                /*string FolderName = Path;
                char[] seperator = { '\\'};
                string[] slipstrings = new string[100];
                slipstrings = FolderName.Split(seperator);
                FolderName = slipstrings[1];*/
                //Cmd = "call C:\\Users\\anvilawang\\Desktop\\1114\\newMultiTaskCsharp_UI\\MultiTaskCsharp\\bin\\Release\\unlocker.exe " + Path + " /S";
                /*Cmd = "call .\\unlocker.exe  " + Dest_Path + " /S";

                fs = new StreamWriter(".\\unlock.bat");
                fs.WriteLine(Cmd);
                fs.Close();

                System.Diagnostics.Process.Start(".\\unlock.bat");
                Thread.Sleep(500);*/


            }

            private void Delete_All_File(string Path)
            {
                if (Directory.Exists(Path))
                {

                    //Unlock_All_File(Path);
                    
                    /*string Dest_Path, Path_dir;

                    WIN32_FIND_DATA FindFileData = new WIN32_FIND_DATA();

                    //WIN32_FIND_DATA FindFileData;


                    System.IntPtr hFind_Copy = INVALID_HANDLE_VALUE;


                    // IntPtr hFind_Copy;

                    Dest_Path = Path + "\\*.*";

                    hFind_Copy = FindFirstFile(Dest_Path, ref FindFileData);

                    if (hFind_Copy == INVALID_HANDLE_VALUE)
                    {
                        //WriteLogFile("Pattern path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        //getch();
                    }

                    //sFormatFileName = ".\\output\\Copy";


                    while (FindNextFile(hFind_Copy, ref FindFileData))
                    {
                        if (FindFileData.cFileName.Equals(@".") || FindFileData.cFileName.Equals(@".."))
                        {
                            continue;
                        }

                        if ((FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes) != 0)
                        {

                            Dest_Path = Path + "\\" + FindFileData.cFileName;
                            Path_dir = Dest_Path;
                            Delete_All_File(Dest_Path);
                            RemoveDirectory(Dest_Path);
                            //System.IO.Directory.Delete(Path_dir, true);
                            //FindNextFile(hFind_Copy, &FindFileData);
                        }
                        else
                        {
                            Dest_Path = Path + "\\" + FindFileData.cFileName;
                            //WriteLogFile("Delete File " + Dest_Path + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                            System.IO.File.Delete(Dest_Path);

                        }

                    }

                    CloseHandle(hFind_Copy);*/

                    int result = 0;

                    SHFILEOPSTRUCT pm = new SHFILEOPSTRUCT();
                    pm.wFunc = wFunc.FO_DELETE;
                    pm.pFrom = Path + '\0';
                    pm.pTo = null;
                    pm.fFlags = FILEOP_FLAGS.FOF_NOCONFIRMATION ;
                    result = SHFileOperation(pm);

                }
                //RemoveDirectory(Path);
            }

            public void Delete_File_Heavy_Loading(byte Ti,string Path_Destination)
            {
                
               /* string delpath_des;
                string strpath_des = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test\\";
                
                if (All != 0)
                {

                  
                    delpath_des = uitaskparameter[Ti - 1].DestDisk + ":\\T" + Ti.ToString() + "_test\\";
                    if (uitaskparameter[Ti - 1].Selected == "enable")
                    {
                       
                        if (Directory.Exists(delpath_des))
                        {
                            System.IO.Directory.Delete(delpath_des, true);
                        }
                        else
                            ;
                    }



               }
                else
                {
                    delpath_des = uitaskparameter[Ti - 1].DestDisk + ":\\T" + Ti.ToString() + "_test\\";
                    if (Directory.Exists(delpath_des))
                    {
                        System.IO.Directory.Delete(delpath_des, true);
                    }
                    /*if (Directory.Exists(strpath_des))
                    {
                        String[] dirs = Directory.GetFiles(strpath_des);
                        List<string> dirlist = new List<string>();
                        foreach (string strFile in dirs)
                            System.IO.File.Delete(strFile);
                    }
                }*/

                string Dest_Path,Path_dir;

                WIN32_FIND_DATA FindFileData = new WIN32_FIND_DATA();

                //WIN32_FIND_DATA FindFileData;


                System.IntPtr hFind_Copy = INVALID_HANDLE_VALUE;


                // IntPtr hFind_Copy;

                Dest_Path = Path_Destination + "\\*.*";

                hFind_Copy = FindFirstFile(Dest_Path, ref FindFileData);

                if (hFind_Copy == INVALID_HANDLE_VALUE)
                {
                    WriteLogFile("Pattern path check, find no file!\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                    //getch();
                }

                //sFormatFileName = ".\\output\\Copy";


                while (FindNextFile(hFind_Copy, ref FindFileData))
                {
                    if (FindFileData.cFileName.Equals(@".") || FindFileData.cFileName.Equals(@".."))
                    {
                        continue;
                    }

                    if ((FILE_ATTRIBUTE_DIRECTORY & FindFileData.dwFileAttributes) != 0)
                    {

                        Dest_Path = Path_Destination + "\\" + FindFileData.cFileName;
                        Path_dir = Dest_Path;
                        Delete_File_Heavy_Loading(Ti, Dest_Path);
                        //System.IO.Directory.Delete(Path_dir, true);
                        //FindNextFile(hFind_Copy, &FindFileData);
                    }
                    else
                    {
                        Dest_Path = Path_Destination + "\\" + FindFileData.cFileName;
                        WriteLogFile("Delete File " + Dest_Path + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                        System.IO.File.Delete(Dest_Path);

                    }

                }

                CloseHandle(hFind_Copy);

            }

        public void CreateLogPath(byte Ti)
        {
            string strpath;
            string currtime;
            string DirePath;
            DateTime CurrentTime = DateTime.Now;
            strpath = "../";
            DirePath = CheckDir(strpath);     //得到上层目录
            strpath = DirePath + "\\output";
            DirePath = CheckDir(strpath);     //在上层目录中建一个output文件夹
            currtime = System.DateTime.Now.Year.ToString() + "_" + System.DateTime.Now.Month.ToString() + "_" + System.DateTime.Now.Day.ToString() + "_" + System.DateTime.Now.Hour.ToString() + "_" + System.DateTime.Now.Minute.ToString() + "_" + System.DateTime.Now.Second.ToString();
            uitaskparameter[Ti - 1].Log_hFile = DirePath + "\\test" + Ti.ToString() + "_" + currtime + ".txt";
            CreateFile(uitaskparameter[Ti - 1].Log_hFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        }

            private void WriteLogFile(string msg, string TargetFile,byte Ti)
            {
                lock (this)
                {
                    byte[] buffer;
                    FileStream MyFile = new FileStream(TargetFile, FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
                    buffer = System.Text.Encoding.Default.GetBytes(msg);
                    MyFile.Write(buffer, 0, buffer.GetLength(0));
                    MyFile.Close();
                    //form2.output.WriteLine(msg);
                    switch (Ti)
                    {
                        case 1:
                            {
                                form2.output_T1.WriteLine(msg);
                            }
                            break;
                        case 2:
                            {
                                form3.output_T2.WriteLine(msg);
                            }
                            break;
                        case 3:
                            {
                                form4.output_T3.WriteLine(msg);
                            }
                            break;
                        case 4:
                            {
                                form5.output_T4.WriteLine(msg);
                            }
                            break;
                        case 5:
                            {
                                form6.output_T5.WriteLine(msg);
                            }
                            break;
                        case 6:
                            {
                                form7.output_T6.WriteLine(msg);
                            }
                            break;
                        case 7:
                            {
                                form8.output_T7.WriteLine(msg);
                            }
                            break;
                        case 8:
                            {
                                form9.output_T8.WriteLine(msg);
                            }
                            break;
                        case 9:
                            {
                                form10.output_T9.WriteLine(msg);
                            }
                            break;
                        case 10:
                            {
                                form11.output_T10.WriteLine(msg);
                            }
                            break;
                    }
                }
            }

            public static UInt64 GetFileSize(string filePath)
         {
             UInt64 temp = 0;
             string[] str1 = Directory.GetFileSystemEntries(filePath);
             foreach (string s1 in str1)
             {
                 FileInfo fileinfo = new FileInfo(s1);
                 temp += (UInt64)fileinfo.Length;
             }
             return temp;
         }                                            

  
      
            private void One_Task(Byte Ti)
            {
                //double TotleFileSizeMbs, Speed;
                //UInt64 TotleFileSize; 
                //long TotleFileSizeMb; 
                DateTime TimeStart_Create, TimeStart_Copy, TimeStart_Compare, TimeStart_Delete, TimeEnd_Delete, TimeEnd_Create, TimeEnd_Copy, TimeEnd_Compare;
                double Time_Create, Time_Copy, Time_Compare, Time_Delete;                

                UInt64 i = 1;
                Test_Function = 0;
                CreateLogPath(Ti);
               // GetAllDriversType(Ti);//
                WriteLogFile("Teststart: " + System.DateTime.Now.ToString("yyyy-MM-dd HH：mm：ss") + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);
                WriteLogFile("Test#: " + Ti.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);
                WriteLogFile("Creating File...\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

                CloseHandle(File_dest[Ti - 1]);
                CloseHandle(File_src[Ti - 1]);

                string Path_Delete = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test";
                Delete_All_File(Path_Delete);
                Path_Delete = uitaskparameter[Ti - 1].SourceDisk + ":\\T" + Ti.ToString() + "_test";
                Delete_All_File(Path_Delete);

                TimeStart_Create = DateTime.Now;
                CreateFile(Ti);
                TimeEnd_Create = DateTime.Now;
                Time_Create = ComputeTime(TimeStart_Create, TimeEnd_Create);
                //TotleFileSizeMb = GetFileSize(uitaskparameter[Ti - 1].SourceDisk + ":\\" + "T" + Ti.ToString() + "_test") / (long)1024 / (long)1024;
                //TotleFileSizeMbs = (double)TotleFileSizeMb;
                //Speed = TotleFileSizeMbs / Time_Create;
                //WriteLogFile("Date: " + TimeStart_Create.ToString("d") + " start: " + TimeStart_Create.ToString("T") + " stop: " + TimeEnd_Create.ToString("T") + " createtime: " + Time_Create.ToString() + "  (" + Speed.ToString() + "Mb/sec)  " + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);
                

                while (true)
                {
                    WriteLogFile("Copy File in cycle" + i.ToString() + "...\r\n " , uitaskparameter[Ti - 1].Log_hFile, Ti);

                    TimeStart_Copy = DateTime.Now;
                    //Copy_File(Ti);
                    Copy_File_Multi_Task(Ti);
                    TimeEnd_Copy = DateTime.Now;
                    Time_Copy = ComputeTime(TimeStart_Copy, TimeEnd_Copy);
                    //Speed = TotleFileSizeMbs / Time_Copy;
                    //WriteLogFile("Date: " + TimeStart_Copy.ToString("d") + " start: " + TimeStart_Copy.ToString("T") + " stop: " + TimeEnd_Copy.ToString("T") + " copytime: " + Time_Copy.ToString() + "  (" + Speed.ToString() + "Mb/sec)  " + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);


                    WriteLogFile("Compare File in cycle" + i.ToString() + "...\r\n ", uitaskparameter[Ti - 1].Log_hFile, Ti);
                    TimeStart_Compare = DateTime.Now;
                    Compare_File_Multi_Task(Ti);
                    TimeEnd_Compare = DateTime.Now;
                    Time_Compare = ComputeTime(TimeStart_Compare, TimeEnd_Compare);
                   // Speed = TotleFileSizeMbs / Time_Compare;
                    //WriteLogFile("Date: " + TimeStart_Compare.ToString("d") + " start: " + TimeStart_Compare.ToString("T") + " stop: " + TimeEnd_Compare.ToString("T") + " comparetime: " + Time_Compare.ToString() + "  (" + Speed.ToString() + "Mb/sec)  " + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);
                   // WriteLogFile("entire transferred capacity till now: " + TotleFileSizeMb.ToString() + "MB" + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);
                    //TotleFileSizeMb += GetFileSize(uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test") / (long)1024 / (long)1024;

                    WriteLogFile("Delete File in cycle" + i.ToString() + "...\r\n ", uitaskparameter[Ti - 1].Log_hFile, Ti);
                    TimeStart_Delete = DateTime.Now;
                    //Delete_File(Ti, 0);
                    Delete_File_Heavy_Loading(Ti, uitaskparameter[Ti - 1].DestDisk + ":\\T" + Ti.ToString() + "_test\\");
                    TimeEnd_Delete = DateTime.Now;
                    Time_Delete = ComputeTime(TimeStart_Delete, TimeEnd_Delete);
                    //Speed = (TotleFileSizeMbs*(double)1024) / (Time_Delete*(double)1024);
                    //WriteLogFile("Date: " + TimeStart_Delete.ToString("d") + " start: " + TimeStart_Delete.ToString("T") + " stop: " + TimeEnd_Delete.ToString("T") + " deletetime: " + Time_Delete.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile,Ti);

                    i++;

                }





            }

            private void One_Task_Create(Byte Ti)
            {
                
                DateTime TimeStart_Create,TimeEnd_Create;
                double Time_Create;

          
                CreateLogPath(Ti);
                // GetAllDriversType(Ti);//
                WriteLogFile("Teststart: " + System.DateTime.Now.ToString("yyyy-MM-dd HH：mm：ss") + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                WriteLogFile("Test#: " + Ti.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                WriteLogFile("Creating File...\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

                string Path_Delete = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test";
                Delete_All_File(Path_Delete);
                Path_Delete = uitaskparameter[Ti - 1].SourceDisk + ":\\T" + Ti.ToString() + "_test";
                Delete_All_File(Path_Delete);

                TimeStart_Create = DateTime.Now;
                CreateFile(Ti);
                TimeEnd_Create = DateTime.Now;
                Time_Create = ComputeTime(TimeStart_Create, TimeEnd_Create);

                WriteLogFile("Finish Create File.\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                Task[Ti - 1].Abort();

            }

            private void One_Task_Heavy_Loading(Byte Ti)
            {

                int Copy_Num_Total,Copy_Num = 0; 
                
                
                Test_Function = 1;
                CreateLogPath(Ti);
                // GetAllDriversType(Ti);//
                WriteLogFile("Teststart: " + System.DateTime.Now.ToString("yyyy-MM-dd HH：mm：ss") + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                WriteLogFile("Test#: " + Ti.ToString() + "\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);
                //WriteLogFile("Copying File...\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

                CloseHandle(File_dest[Ti - 1]);
                CloseHandle(File_src[Ti - 1]);

                string Path_Delete = uitaskparameter[Ti - 1].DestDisk + ":\\" + "T" + Ti.ToString() + "_test";
                Delete_All_File(Path_Delete);

                Copy_Num_Total = Compute_Copy_Number();
                WriteLogFile("Copy Loop is " + Copy_Num_Total.ToString() + ".\r\n", uitaskparameter[Ti - 1].Log_hFile, Ti);

               

                while (Copy_Num < Copy_Num_Total)
                {
                    WriteLogFile("Copy File in cycle" + Copy_Num.ToString() + "...\r\n ", uitaskparameter[Ti - 1].Log_hFile, Ti);

                    Copy_File_HeavyLoading(Ti, Copy_Num);

                    WriteLogFile("Compare File in cycle" + Copy_Num.ToString() + "...\r\n ", uitaskparameter[Ti - 1].Log_hFile, Ti);

                    Compare_File_Heavy_Loading(Ti, Copy_Num);

                    Copy_Num++;

                    if (Copy_Num == Copy_Num_Total)
                    {
                        Delete_File_Heavy_Loading(Ti, uitaskparameter[Ti - 1].DestDisk + ":\\T" + Ti.ToString() + "_test\\");
                        Copy_Num = 0;
                    }
                    

                }

                



            }


            private void Start_Click(object sender, EventArgs e)
            {


                int i,j;
                string Task_Message;

                int xWidth = SystemInformation.PrimaryMonitorSize.Width;//获取显示器屏幕宽度
 
                int yHeight = SystemInformation.PrimaryMonitorSize.Height;//高度

                int x = 0,y = 0;
 
               

                GetUIMsg();

            
                Task[0] = new Thread(new ThreadStart(delegate { One_Task(1); }));
                Task[1] = new Thread(new ThreadStart(delegate { One_Task(2); }));
                Task[2] = new Thread(new ThreadStart(delegate { One_Task(3); }));
                Task[3] = new Thread(new ThreadStart(delegate { One_Task(4); }));
                Task[4] = new Thread(new ThreadStart(delegate { One_Task(5); }));
                Task[5] = new Thread(new ThreadStart(delegate { One_Task(6); }));
                Task[6] = new Thread(new ThreadStart(delegate { One_Task(7); }));
                Task[7] = new Thread(new ThreadStart(delegate { One_Task(8); }));
                Task[8] = new Thread(new ThreadStart(delegate { One_Task(9); }));
                Task[9] = new Thread(new ThreadStart(delegate { One_Task(10); }));
                /*
                Task[0] = new Thread(new ThreadStart(delegate { One_Task_Change(1); }));
                Task[1] = new Thread(new ThreadStart(delegate { One_Task_Change(2); }));
                Task[2] = new Thread(new ThreadStart(delegate { One_Task_Change(3); }));
                Task[3] = new Thread(new ThreadStart(delegate { One_Task_Change(4); }));
                Task[4] = new Thread(new ThreadStart(delegate { One_Task_Change(5); }));
                Task[5] = new Thread(new ThreadStart(delegate { One_Task_Change(6); }));
                Task[6] = new Thread(new ThreadStart(delegate { One_Task_Change(7); }));
                Task[7] = new Thread(new ThreadStart(delegate { One_Task_Change(8); }));
                Task[8] = new Thread(new ThreadStart(delegate { One_Task_Change(9); }));
                Task[9] = new Thread(new ThreadStart(delegate { One_Task_Change(10); }));
            */
                

                for (i = 0; i < 10; i++)
                {
                    if (uitaskparameter[i].Selected == "enable")
                    {
                        Task[i].Start();
                        j = i + 1;


                        switch (j)
                        {

                            case 1:
                                {
                                    form2 = new Form2(this);

                                    form2.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form2.output_T1.WriteLine(Task_Message);

                                    form2.StartPosition = FormStartPosition.Manual;
                                    form2.Location = new Point(x,y);
                                    
                                    y += 190;
                                    if(y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form2.Show();
                                }
                                break;
                            case 2:
                                {
                                    form3 = new Form3(this);

                                    form3.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form3.output_T2.WriteLine(Task_Message);
                                    form3.StartPosition = FormStartPosition.Manual;
                                    form3.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form3.Show();
                                }
                                break;
                            case 3:
                                {
                                    form4 = new Form4(this);

                                    form4.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form4.output_T3.WriteLine(Task_Message);
                                    form4.StartPosition = FormStartPosition.Manual;
                                    form4.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form4.Show();
                                }
                                break;
                            case 4:
                                {
                                    form5 = new Form5(this);

                                    form5.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form5.output_T4.WriteLine(Task_Message);
                                    form5.StartPosition = FormStartPosition.Manual;
                                    form5.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form5.Show();
                                }
                                break;
                            case 5:
                                {
                                    form6 = new Form6(this);

                                    form6.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form6.output_T5.WriteLine(Task_Message);
                                    form6.StartPosition = FormStartPosition.Manual;
                                    form6.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form6.Show();
                                }
                                break;
                            case 6:
                                {
                                    form7 = new Form7(this);

                                    form7.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form7.output_T6.WriteLine(Task_Message);
                                    form7.StartPosition = FormStartPosition.Manual;
                                    form7.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form7.Show();
                                }
                                break;
                            case 7:
                                {
                                    form8 = new Form8(this);

                                    form8.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form8.output_T7.WriteLine(Task_Message);
                                    form8.StartPosition = FormStartPosition.Manual;
                                    form8.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form8.Show();
                                }
                                break;
                            case 8:
                                {
                                    form9 = new Form9(this);

                                    form9.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form9.output_T8.WriteLine(Task_Message);
                                    form9.StartPosition = FormStartPosition.Manual;
                                    form9.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form9.Show();
                                }
                                break;
                            case 9:
                                {
                                    form10 = new Form10(this);

                                    form10.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form10.output_T9.WriteLine(Task_Message);
                                    form10.StartPosition = FormStartPosition.Manual;
                                    form10.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form10.Show();
                                }
                                break;
                            case 10:
                                {
                                    form11 = new Form11(this);

                                    form11.Text = "Task " + j.ToString();

                                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                    form11.output_T10.WriteLine(Task_Message);
                                    form11.StartPosition = FormStartPosition.Manual;
                                    form11.Location = new Point(x, y);

                                    y += 190;
                                    if (y >= (yHeight - 190))
                                    {
                                        x = x + 308;
                                        y = 0;
                                    }

                                    form11.Show();
                                }
                                break;
                        }
                        /*form2 = new Form2();
                        
                        form2.Text = "Task " + j.ToString();

                        Task_Message = "Task " + j.ToString() + " is beginning! \n";

                        form2.output_T1.WriteLine(Task_Message);
                        
                        form2.Show();*/
                    }
                }
               


            }

            public void button2_Click(object sender, EventArgs e)
            {

                int i,j;
                bool result;
                string Path;

                for (i = 0; i < 10; i++)
                {
                    if (uitaskparameter[i].Selected == "enable")
                    {
                        result = CloseHandle(File_dest[i]);
                        result = CloseHandle(File_src[i]);
                        j = i + 1;
                        Task[i].Abort();
                        Path = uitaskparameter[i].DestDisk + ":\\T" + j.ToString() + "_test";
                        Unlock_All_File(Path);
                        Path = uitaskparameter[i].SourceDisk + ":\\T" + j.ToString() + "_test";
                        Unlock_All_File(Path);
                        
                        //Delete_File((Byte)j, (Byte)1);
                        switch (j)
                        {
                            case 1:
                                {
                                    form2.Close();
                                }
                                break;
                            case 2:
                                {
                                    form3.Close();
                                }
                                break;
                            case 3:
                                {
                                    form4.Close();
                                }
                                break;
                            case 4:
                                {
                                    form5.Close();
                                }
                                break;
                            case 5:
                                {
                                    form6.Close();
                                }
                                break;
                            case 6:
                                {
                                    form7.Close();
                                }
                                break;
                            case 7:
                                {
                                    form8.Close();
                                }
                                break;
                            case 8:
                                {
                                    form9.Close();
                                }
                                break;
                            case 9:
                                {
                                    form10.Close();
                                }
                                break;
                            case 10:
                                {
                                    form11.Close();
                                }
                                break;      
                        }
                    }
                }

            }

        private void T1_Select_CheckedChanged(object sender, EventArgs e)
        {
           // GetAllDriversType(1);
        }

        private void button3_Click(object sender, EventArgs e)
        {
            byte i = 1;
            int j = 0;
            CheckBox checkbox;
            //RadioButton radiobutton;
            TextBox textbox;
            ComboBox combobox;
            char[] Arry_path;

            while (i <= 10)
            {
                checkbox = this.Controls.Find("T" + i.ToString() + "_Select", true)[0] as CheckBox;
                if (checkbox.Checked == true)
                {
                    WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Selected", "enable", ".\\UISetting.ini");
                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Src", true)[0];

                    if (combobox.SelectedIndex == -1)
                    {
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".SourceDisk", "D", ".\\UISetting.ini");
                    }
                    else
                    {
                        Arry_path = combobox.Text.ToCharArray();
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".SourceDisk", Arry_path[0].ToString(), ".\\UISetting.ini");
                    }

                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Dest", true)[0];
                    if (combobox.SelectedIndex == -1)
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".DestDisk", "D", ".\\UISetting.ini");
                    else
                    {
                        Arry_path = combobox.Text.ToCharArray();
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".DestDisk", Arry_path[0].ToString(), ".\\UISetting.ini");
                    }

                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Pattern", true)[0];
                    if (combobox.SelectedIndex == -1)
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Pattern", "Random", ".\\UISetting.ini");
                    else
                        //combobox.SelectedValue 
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Pattern", combobox.Text , ".\\UISetting.ini");

                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_FileSize", true)[0];
                    if (combobox.SelectedIndex == -1)
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".FileSize", "1*50MB", ".\\UISetting.ini");
                    else
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".FileSize", combobox.Text, ".\\UISetting.ini");

                    textbox = (TextBox)this.Controls.Find("T" + i.ToString() + "_Time", true)[0];
                    WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Time", textbox.Text, ".\\UISetting.ini");
                    if (uitaskparameter[i - 1].Time == "")
                        WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Time", "10", ".\\UISetting.ini");
                    i++;
                }
                else
                {
                    WritePrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Selected", "disable", ".\\UISetting.ini");
                    i++;
                    j++;
                }
            }
            if (j == 10)
            {
                MessageBox.Show("None information to save to ini file.");
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            byte i = 1;
            int j = 0,k;
            CheckBox checkbox;
            TextBox textbox;
            ComboBox combobox;
            char[] Arry_path;
            StringBuilder temp = new StringBuilder(255);
            //Form1 form1;
            string tempvalue,boxvalue;

            while (i <= 10)
            {
                GetPrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Selected", "disable", temp, 255, ".\\UISetting.ini");
                tempvalue = temp.ToString();
                checkbox = this.Controls.Find("T" + i.ToString() + "_Select", true)[0] as CheckBox;
                if (tempvalue == "enable")
                {
                    checkbox.Checked = true;
                    GetPrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".SourceDisk", "D", temp, 255, ".\\UISetting.ini");
                    tempvalue = temp.ToString();
                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Src", true)[0];
                    for (k = 0; k < 100; k++)
                    {
                        combobox.SelectedIndex = k;
                        Arry_path = combobox.Text.ToCharArray();
                        boxvalue = Arry_path[0].ToString();
                        if (boxvalue == tempvalue)
                            break;
                    }
                    GetPrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".DestDisk", "E", temp, 255, ".\\UISetting.ini");
                    tempvalue = temp.ToString();
                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Dest", true)[0];
                    for (k = 0; k < 100; k++)
                    {
                        combobox.SelectedIndex = k;
                        Arry_path = combobox.Text.ToCharArray();
                        boxvalue = Arry_path[0].ToString();
                        if (boxvalue == tempvalue)
                            break;
                    }
                    GetPrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Pattern", "Random", temp, 255, ".\\UISetting.ini");
                    tempvalue = temp.ToString();
                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_Pattern", true)[0];
                    for (k = 0; k < 100; k++)
                    {
                        combobox.SelectedIndex = k;
                        boxvalue = combobox.Text.ToString();
                        if (boxvalue == tempvalue)
                            break;
                    }
                    GetPrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".FileSize", "1*50MB", temp, 255, ".\\UISetting.ini");
                    tempvalue = temp.ToString();
                    combobox = (ComboBox)Controls.Find("T" + i.ToString() + "_FileSize", true)[0];
                    for (k = 0; k < 100; k++)
                    {
                        combobox.SelectedIndex = k;
                        boxvalue = combobox.Text.ToString();
                        if (boxvalue == tempvalue)
                            break;
                    }
                    GetPrivateProfileString("Task " + i.ToString(), "T" + i.ToString() + ".Time", "10", temp, 255, ".\\UISetting.ini");
                    tempvalue = temp.ToString();
                    textbox = (TextBox)this.Controls.Find("T" + i.ToString() + "_Time", true)[0];
                    textbox.Text = tempvalue;
                    i++;
                }
                else
                {
                    checkbox.Checked = false;
                    i++;
                    j++;
                }
            }
            if (j == 10)
            {
                MessageBox.Show("None information to load from ini file.");
            }
        }

        private void Start_HL_Click(object sender, EventArgs e)
        {
            int i, j;
            string Task_Message;

            int xWidth = SystemInformation.PrimaryMonitorSize.Width;//获取显示器屏幕宽度

            int yHeight = SystemInformation.PrimaryMonitorSize.Height;//高度

            int x = 0, y = 0;



            GetUIMsg();
        
            //Create_Source_File_Heavy_Loading();
            Create_Source_File_Heavy_Loading_Special();


            Task[0] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(1); }));
            Task[1] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(2); }));
            Task[2] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(3); }));
            Task[3] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(4); }));
            Task[4] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(5); }));
            Task[5] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(6); }));
            Task[6] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(7); }));
            Task[7] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(8); }));
            Task[8] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(9); }));
            Task[9] = new Thread(new ThreadStart(delegate { One_Task_Heavy_Loading(10); }));
            

            for (i = 0; i < 10; i++)
            {
                if (uitaskparameter[i].Selected == "enable")
                {
                    Task[i].Start();
                    j = i + 1;


                    switch (j)
                    {

                        case 1:
                            {
                                form2 = new Form2(this);

                                form2.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form2.output_T1.WriteLine(Task_Message);

                                form2.StartPosition = FormStartPosition.Manual;
                                form2.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form2.Show();
                            }
                            break;
                        case 2:
                            {
                                form3 = new Form3(this);

                                form3.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form3.output_T2.WriteLine(Task_Message);
                                form3.StartPosition = FormStartPosition.Manual;
                                form3.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form3.Show();
                            }
                            break;
                        case 3:
                            {
                                form4 = new Form4(this);

                                form4.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form4.output_T3.WriteLine(Task_Message);
                                form4.StartPosition = FormStartPosition.Manual;
                                form4.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form4.Show();
                            }
                            break;
                        case 4:
                            {
                                form5 = new Form5(this);

                                form5.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form5.output_T4.WriteLine(Task_Message);
                                form5.StartPosition = FormStartPosition.Manual;
                                form5.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form5.Show();
                            }
                            break;
                        case 5:
                            {
                                form6 = new Form6(this);

                                form6.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form6.output_T5.WriteLine(Task_Message);
                                form6.StartPosition = FormStartPosition.Manual;
                                form6.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form6.Show();
                            }
                            break;
                        case 6:
                            {
                                form7 = new Form7(this);

                                form7.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form7.output_T6.WriteLine(Task_Message);
                                form7.StartPosition = FormStartPosition.Manual;
                                form7.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form7.Show();
                            }
                            break;
                        case 7:
                            {
                                form8 = new Form8(this);

                                form8.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form8.output_T7.WriteLine(Task_Message);
                                form8.StartPosition = FormStartPosition.Manual;
                                form8.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form8.Show();
                            }
                            break;
                        case 8:
                            {
                                form9 = new Form9(this);

                                form9.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form9.output_T8.WriteLine(Task_Message);
                                form9.StartPosition = FormStartPosition.Manual;
                                form9.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form9.Show();
                            }
                            break;
                        case 9:
                            {
                                form10 = new Form10(this);

                                form10.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form10.output_T9.WriteLine(Task_Message);
                                form10.StartPosition = FormStartPosition.Manual;
                                form10.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form10.Show();
                            }
                            break;
                        case 10:
                            {
                                form11 = new Form11(this);

                                form11.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form11.output_T10.WriteLine(Task_Message);
                                form11.StartPosition = FormStartPosition.Manual;
                                form11.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form11.Show();
                            }
                            break;
                    }

                    
                }
            }
        }

        private void Stop_HL_Click(object sender, EventArgs e)
        {
            int i, j;
            string Path;

            //Stop_Test(1);
            for (i = 0; i < 10; i++)
            {
                if (uitaskparameter[i].Selected == "enable")
                {
                    Path = uitaskparameter[i].SourceDisk + ":\\S";
                    Unlock_All_File(Path);
                    break;
                }
            }

            for (i = 0; i < 10; i++)
            {
                if (uitaskparameter[i].Selected == "enable")
                {
                    Task[i].Abort();
                    CloseHandle(File_dest[i]);
                    CloseHandle(File_src[i]);
                    j = i + 1;

                    Path = uitaskparameter[i].DestDisk + ":\\T" + j.ToString() + "_test";
                    Unlock_All_File(Path);
                    
                    //Delete_File((Byte)j, (Byte)1);
                    switch (j)
                    {
                        case 1:
                            {
                                form2.Close();
                            }
                            break;
                        case 2:
                            {
                                form3.Close();
                            }
                            break;
                        case 3:
                            {
                                form4.Close();
                            }
                            break;
                        case 4:
                            {
                                form5.Close();
                            }
                            break;
                        case 5:
                            {
                                form6.Close();
                            }
                            break;
                        case 6:
                            {
                                form7.Close();
                            }
                            break;
                        case 7:
                            {
                                form8.Close();
                            }
                            break;
                        case 8:
                            {
                                form9.Close();
                            }
                            break;
                        case 9:
                            {
                                form10.Close();
                            }
                            break;
                        case 10:
                            {
                                form11.Close();
                            }
                            break;
                    }
                    
                }
            }
            
        }

        private void T4_FileSize_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void T1_Pattern_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void Create_File_Click(object sender, EventArgs e)
        {
            int i,j;
            string Task_Message;

            int xWidth = SystemInformation.PrimaryMonitorSize.Width;//获取显示器屏幕宽度

            int yHeight = SystemInformation.PrimaryMonitorSize.Height;//高度

            int x = 0,y = 0;

           

            GetUIMsg();

        
            Task[0] = new Thread(new ThreadStart(delegate { One_Task_Create(1); }));
            Task[1] = new Thread(new ThreadStart(delegate { One_Task_Create(2); }));
            Task[2] = new Thread(new ThreadStart(delegate { One_Task_Create(3); }));
            Task[3] = new Thread(new ThreadStart(delegate { One_Task_Create(4); }));
            Task[4] = new Thread(new ThreadStart(delegate { One_Task_Create(5); }));
            Task[5] = new Thread(new ThreadStart(delegate { One_Task_Create(6); }));
            Task[6] = new Thread(new ThreadStart(delegate { One_Task_Create(7); }));
            Task[7] = new Thread(new ThreadStart(delegate { One_Task_Create(8); }));
            Task[8] = new Thread(new ThreadStart(delegate { One_Task_Create(9); }));
            Task[9] = new Thread(new ThreadStart(delegate { One_Task_Create(10); }));
           


            for (i = 0; i < 10; i++)
            {
                if (uitaskparameter[i].Selected == "enable")
                {
                    Task[i].Start();
                    j = i + 1;


                    switch (j)
                    {

                        case 1:
                            {
                                form2 = new Form2(this);

                                form2.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form2.output_T1.WriteLine(Task_Message);

                                form2.StartPosition = FormStartPosition.Manual;
                                form2.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form2.Show();
                            }
                            break;
                        case 2:
                            {
                                form3 = new Form3(this);

                                form3.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form3.output_T2.WriteLine(Task_Message);
                                form3.StartPosition = FormStartPosition.Manual;
                                form3.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form3.Show();
                            }
                            break;
                        case 3:
                            {
                                form4 = new Form4(this);

                                form4.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form4.output_T3.WriteLine(Task_Message);
                                form4.StartPosition = FormStartPosition.Manual;
                                form4.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form4.Show();
                            }
                            break;
                        case 4:
                            {
                                form5 = new Form5(this);

                                form5.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form5.output_T4.WriteLine(Task_Message);
                                form5.StartPosition = FormStartPosition.Manual;
                                form5.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form5.Show();
                            }
                            break;
                        case 5:
                            {
                                form6 = new Form6(this);

                                form6.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form6.output_T5.WriteLine(Task_Message);
                                form6.StartPosition = FormStartPosition.Manual;
                                form6.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form6.Show();
                            }
                            break;
                        case 6:
                            {
                                form7 = new Form7(this);

                                form7.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form7.output_T6.WriteLine(Task_Message);
                                form7.StartPosition = FormStartPosition.Manual;
                                form7.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form7.Show();
                            }
                            break;
                        case 7:
                            {
                                form8 = new Form8(this);

                                form8.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form8.output_T7.WriteLine(Task_Message);
                                form8.StartPosition = FormStartPosition.Manual;
                                form8.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form8.Show();
                            }
                            break;
                        case 8:
                            {
                                form9 = new Form9(this);

                                form9.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form9.output_T8.WriteLine(Task_Message);
                                form9.StartPosition = FormStartPosition.Manual;
                                form9.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form9.Show();
                            }
                            break;
                        case 9:
                            {
                                form10 = new Form10(this);

                                form10.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form10.output_T9.WriteLine(Task_Message);
                                form10.StartPosition = FormStartPosition.Manual;
                                form10.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form10.Show();
                            }
                            break;
                        case 10:
                            {
                                form11 = new Form11(this);

                                form11.Text = "Task " + j.ToString();

                                Task_Message = "Task " + j.ToString() + " is beginning! \n";

                                form11.output_T10.WriteLine(Task_Message);
                                form11.StartPosition = FormStartPosition.Manual;
                                form11.Location = new Point(x, y);

                                y += 190;
                                if (y >= (yHeight - 190))
                                {
                                    x = x + 308;
                                    y = 0;
                                }

                                form11.Show();
                            }
                            break;
                    }
                    /*form2 = new Form2();
                    
                    form2.Text = "Task " + j.ToString();

                    Task_Message = "Task " + j.ToString() + " is beginning! \n";

                    form2.output_T1.WriteLine(Task_Message);
                    
                    form2.Show();*/
                }
            }
        }

        
        }
}
