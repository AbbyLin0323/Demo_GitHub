using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Runtime.InteropServices;
namespace WindowsFormsApplication4
{
    public interface ITestMethod
    {
        /*virtual void Copy_file(string TargetFPath);
        virtual void Compare_file(string TargetFPath);
        virtual void Delete_file(string TargetFPath);*/
    }
}

namespace WindowsFormsApplication4
{
   public class CTestBase
    {
       protected bool bDisk_PowerOff;
       protected int ucRealFileNum;
       protected int ucFileLostNum;
       protected int ucFileFailCMP;
       protected int ucCycleFileNum;
       protected int ucFileFailCMP_Senior;
       protected bool bDisk_Fail;
       protected CCMPSetting refCMPSetting;

       public CTestBase(ref CCMPSetting refcmpSetting_Arg)
       {
           refCMPSetting = refcmpSetting_Arg;
       }

       public CTestBase()
       {
           ucFileFailCMP_Senior = 0;
           bDisk_Fail = false;
           bDisk_PowerOff = false;
           ucRealFileNum = 0;
           ucFileLostNum = 0;
           ucFileFailCMP = 0;
           ucCycleFileNum = 0;
       }
       public int _FILE_CMP_Senior()
       {
           return ucFileFailCMP_Senior;
       }
       public bool _Disk_Fail()
       {
           return bDisk_Fail;
       }

       public void ClearOneCycle()
       {
           this.ucCycleFileNum = 0;
           this.ucFileFailCMP = 0;
           this.ucFileLostNum = 0;
           this.bDisk_Fail = false;
       }

       public int _Cycle_File_Num()
       {
           return ucCycleFileNum;
       }

     
       public int _FileFailCMP()
       {
           return ucFileFailCMP;
       }

       public void _Disk_PowerOff(bool bValue)
       {
           bDisk_PowerOff = bValue;
       }

       public int _RealFileNum()
       {
           return ucRealFileNum;
       }

       public int _FileLostNum()
       {
           return ucFileLostNum;
       }

       public virtual void Create_file(ref string strTargetPath, ref CSetting pSetting) { }
        public virtual void Copy_file(string TargetFPath, ref CFileItem refFI){}
        public virtual void Compare_file(string TargetFPath, ref CFileItem refFI) {  }
        public virtual void Delete_file(string TargetFPath, ref CFileItem refFI) { }
        public string GetDstFileName(ref string fromFile, int nHowManyTimes)
        {
            //string fromFile;
            // string fromFile = refFI.FList[nIndex];
            //fromFile = "c:\\1g\\0xaa\\0.srctesttmp";
            string strFirst = fromFile.Substring(0, fromFile.LastIndexOf("\\"));
            string strValue = strFirst.Substring(strFirst.LastIndexOf("\\") + 1, strFirst.Length - strFirst.LastIndexOf("\\") - 1);
            string strSecond = strFirst.Substring(0, strFirst.LastIndexOf("\\"));
            string strSize = strSecond.Substring(strSecond.LastIndexOf("\\") + 1, strSecond.Length - strSecond.LastIndexOf("\\") - 1);
            string srcFirstName = fromFile.Substring(fromFile.LastIndexOf("\\") + 1, (fromFile.LastIndexOf(".") - fromFile.LastIndexOf("\\") - 1));  //文件名
            string strDstName;
            if (-1 != nHowManyTimes)
                strDstName = strSize + strValue + srcFirstName + "+" + nHowManyTimes.ToString() + ".dsttesttmp";
            else
                strDstName = strSize + strValue + srcFirstName + ".dsttesttmp";
            return strDstName;
        }
        public bool IsNeedCompare(ref string fromFile, ref string strDstFileName)
        {
            bool bret = false;
            string construct_dstname = GetDstFileName(ref fromFile, -1);
            string dstFirstName = strDstFileName.Substring(strDstFileName.LastIndexOf("\\") + 1, (strDstFileName.LastIndexOf(".") - strDstFileName.LastIndexOf("\\") - 1));  //文件名
            string extLastName = strDstFileName.Substring(strDstFileName.LastIndexOf(".") + 1, (strDstFileName.Length - strDstFileName.LastIndexOf(".") - 1));   //扩展名
            dstFirstName = dstFirstName.Substring(0, dstFirstName.LastIndexOf("+"));
            dstFirstName = dstFirstName + "." + extLastName;
            if (construct_dstname == dstFirstName)
                bret = true;
            return bret;
        }
      
    }
}

namespace WindowsFormsApplication4
{
    public class FileComparer : IComparer<FileInfo>
    {
        public int Compare(FileInfo o1, FileInfo o2)
        {
            return o1.Name.CompareTo(o2.Name);
        }
    }

    class CMediumTMethod : CTestBase
    {
        [DllImport("kernel32.dll")]
        public static extern IntPtr _lopen(string lpPathName, int iReadWrite);

        [DllImport("kernel32.dll")]
        public static extern bool CloseHandle(IntPtr hObject);

        public const int OF_READWRITE = 2;
        public const int OF_SHARE_DENY_NONE = 0x40;
        public readonly IntPtr HFILE_ERROR = new IntPtr(-1);

        protected int ucFileTimes;
        
        public CMediumTMethod()
        {
        }
        
        
        public CMediumTMethod(int ucFileTimes_Arg, ref CCMPSetting refCMPSetting_Arg){
            
            ucFileLostNum = 0;
            
            ucFileTimes = ucFileTimes_Arg;
            
            ucFileFailCMP = 0;
            bDisk_PowerOff = false;
            ucRealFileNum = 0;
            refCMPSetting = refCMPSetting_Arg;
        }

        
        public override void Create_file(ref string strTargetPath, ref CSetting st) {
            string strCurPath = System.IO.Directory.GetCurrentDirectory();
            string strBigSize, strMediumSize, strSmallSize;
            string strBigNum, strMediumNum, strSmallNum;
            strBigSize = st.size_big.ToString();
            strBigNum = st.num_big.ToString();
            strMediumSize = st.size_medium.ToString();
            strMediumNum = st.num_medium.ToString();
            strSmallSize = st.size_small.ToString();
            strSmallNum = st.num_small.ToString();
            if (strCurPath[strCurPath.Length - 1] != '\\') //非根目录
                strCurPath += "\\";
            string strOperPath = "";
            string strPathValue = "";
            byte[] valueToFile = new byte[1024];
            switch(st.value_big)
            {
                case 0:
                    strPathValue = "0xaa";
                    int nIndex = 0;
                    for ( nIndex = 0; nIndex < 1024; nIndex++)
                        valueToFile[nIndex] = 0xaa;
                    break;
                case 1:
                    strPathValue = "0x55";
                    int nIndex1 = 0;
                    for ( nIndex1 = 0; nIndex1 < 1024; nIndex1++)
                        valueToFile[nIndex1] = 0x55;
                    break;
                case 2:
                    strPathValue = "random value";
                    valueToFile[0] = 0xFF;
                    break;
            }
            strOperPath = strCurPath + strBigSize + "G" + "\\" + strPathValue;
            if (!Directory.Exists(strCurPath + strBigSize +"G"))
                Directory.CreateDirectory(strCurPath + strBigSize + "G");
            if (!Directory.Exists(strOperPath))
                Directory.CreateDirectory(strOperPath);//创建新路径
            DirectoryInfo di = new DirectoryInfo(strOperPath);
            FileInfo[] fi = di.GetFiles("*.srctesttmp");
            Create_Step_File(ref strOperPath, (int)fi.Length, (int)st.num_big, ref valueToFile, st.size_big * 1024 * 1024 * 1024, ref st);

            switch(st.value_medium)
            {
                case 0:
                    strPathValue = "0xaa";
                    int nIndex = 0;
                    for ( nIndex = 0; nIndex < 1024; nIndex++)
                        valueToFile[nIndex] = 0xaa;
                    break;
                case 1:
                    strPathValue = "0x55";
                    int nIndex1 = 0;
                    for ( nIndex1 = 0; nIndex1 < 1024; nIndex1++)
                        valueToFile[nIndex1] = 0x55;
                    break;
                case 2:
                    strPathValue = "random value";
                    valueToFile[0] = 0xFF;
                    break;
            }
            strOperPath = strCurPath + strMediumSize + "M" + "\\" + strPathValue;
            if (!Directory.Exists(strCurPath + strMediumSize + "M"))
                Directory.CreateDirectory(strCurPath + strMediumSize + "M");
            if (!Directory.Exists(strOperPath))
                Directory.CreateDirectory(strOperPath);//创建新路径
            di = new DirectoryInfo(strOperPath);
            fi = di.GetFiles("*.srctesttmp");
            Create_Step_File(ref strOperPath, (int)fi.Length, (int)st.num_medium, ref valueToFile, st.size_medium * 1024 * 1024, ref st);

             switch(st.value_small)
            {
                case 0:
                    strPathValue = "0xaa";
                    int nIndex = 0;
                    for ( nIndex = 0; nIndex < 1024; nIndex++)
                        valueToFile[nIndex] = 0xaa;
                    break;
                case 1:
                    strPathValue = "0x55";
                    int nIndex1 = 0;
                    for ( nIndex1 = 0; nIndex1 < 1024; nIndex1++)
                        valueToFile[nIndex1] = 0x55;
                    break;
                case 2:
                    strPathValue = "random value";
                    valueToFile[0] = 0xFF;
                    break;
            }
            strOperPath = strCurPath + strSmallSize + "K" + "\\" + strPathValue;
            if (!Directory.Exists(strCurPath + strSmallSize + "K"))
                Directory.CreateDirectory(strCurPath + strSmallSize + "K");
            if (!Directory.Exists(strOperPath))
                Directory.CreateDirectory(strOperPath);//创建新路径
            di = new DirectoryInfo(strOperPath);
            fi = di.GetFiles("*.srctesttmp");
            Create_Step_File(ref strOperPath, (int)fi.Length, (int)st.num_small, ref valueToFile, st.size_small * 1024, ref st);
        }

        protected void Create_Step_File(ref string strOperPath, int nLength, int nTotalNum, ref byte[] valueToFile, long filesize,  ref CSetting st)
        {
            if (nLength < nTotalNum)
            {
                int nFileIndex = 0;
                string strFileName;
                long lFilePtr = 0;
                
                for (nFileIndex = nLength + 1; nFileIndex <= nTotalNum; nFileIndex++)
                {
                    strFileName = "\\" + nFileIndex.ToString() + ".srctesttmp";
                    strFileName = strOperPath + "\\" + strFileName;
                    FileStream fstream = File.Create(strFileName);
                    for (lFilePtr = 0; lFilePtr < filesize / 1024; lFilePtr++)
                    {
                        if (valueToFile[0] == 0xFF)
                        {
                            //Write the random value to the file
                            Random ri = new Random();
                            int nRndIndex = 0;
                            for (nRndIndex = 1; nRndIndex < 1024; nRndIndex++)
                            {
                                valueToFile[nRndIndex] = (byte)(ri.Next() * ri.Next() * ri.Next());
                            }
                        }
                        fstream.Write(valueToFile, 0, 1024);
                        fstream.Seek(lFilePtr * 1024, SeekOrigin.Begin);
                    }
                    fstream.Close();
                }
            }
           
        }

        protected void CopySingleFile(ref CFileItem refFI, int nIndex, string TargetFPath, int nHowManyTimes)//string fromFile, string toFile, int lengthEachTime)
        {
            string fromFile = refFI.FList[nIndex];
            string strDstFileName = GetDstFileName(ref fromFile, nHowManyTimes);
            string toFile = TargetFPath + "\\" + strDstFileName;
            long lengthToCopy;
            int lengthEachTime = 1024;
            FileStream fileToCopy = null;
            FileStream copyToFile = null;
            try
            {
                fileToCopy = new FileStream(fromFile, FileMode.Open, FileAccess.Read);
                copyToFile = new FileStream(toFile, FileMode.Append, FileAccess.Write);
                ucRealFileNum++;
                ucCycleFileNum++;
                refFI.strWhichFileName = strDstFileName;
                if (lengthEachTime < fileToCopy.Length)//如果分段拷贝，即每次拷贝内容小于文件总长度
                {
                    byte[] buffer = new byte[lengthEachTime];
                    long copied = 0;
                    while (copied <= (fileToCopy.Length - (long)lengthEachTime))//拷贝主体部分
                    {
                        lengthToCopy = fileToCopy.Read(buffer, 0, lengthEachTime);
                        fileToCopy.Flush();
                        copyToFile.Write(buffer, 0, lengthEachTime);
                        copyToFile.Flush();
                        copyToFile.Position = fileToCopy.Position;
                        copied += lengthToCopy;
                        refFI.uFileSeekPointer = copied;
                    }
                    int left = (int)(fileToCopy.Length - copied);//拷贝剩余部分
                    lengthToCopy = fileToCopy.Read(buffer, 0, left);
                    fileToCopy.Flush();
                    copyToFile.Write(buffer, 0, left);
                    copyToFile.Flush();
                    refFI.uFileSeekPointer = fileToCopy.Length;
                }
                else//如果整体拷贝，即每次拷贝内容大于文件总长度
                {
                    byte[] buffer = new byte[fileToCopy.Length];
                    fileToCopy.Read(buffer, 0, (int)fileToCopy.Length);
                    fileToCopy.Flush();
                    copyToFile.Write(buffer, 0, (int)fileToCopy.Length);
                    copyToFile.Flush();
                    refFI.uFileSeekPointer = fileToCopy.Length;
                }
                fileToCopy.Dispose();
                copyToFile.Dispose();
                fileToCopy.Close();
                copyToFile.Close();

            }
            catch (IOException e)
            {
                return;
            }
            catch (Exception ex)
            {
                if (fileToCopy != null)
                {
                    fileToCopy.Dispose();
                    fileToCopy.Close();

                }
                if (copyToFile != null)
                {
                    copyToFile.Dispose();
                    copyToFile.Close();
                }
                return;
            }
            finally
            {
                if(fileToCopy != null)
                    fileToCopy.Dispose();
                if (copyToFile != null)
                    copyToFile.Dispose();
            }
            
        }

        public override void Copy_file(string TargetFPath, ref CFileItem refFI)
        { 
            Console.WriteLine("call the CMediumTMethod\n");
            int nFileIndex = 0;
            int nFileTimesIndex = 0;
            for (nFileTimesIndex = 0; nFileTimesIndex < ucFileTimes; nFileTimesIndex++)
            {
                for (nFileIndex = 0; nFileIndex < refFI.FList.Count; nFileIndex++)
                {
                    if (bDisk_PowerOff)
                        return;
                    refFI.uWhichFile = nFileIndex;
                    CopySingleFile(ref refFI, nFileIndex, TargetFPath, nFileTimesIndex);
                }
            }
        }

        public override void Compare_file(string TargetFPath, ref CFileItem refFI)
        {
            if (TargetFPath[TargetFPath.Length - 1] != '\\') //非根目录
                TargetFPath += "\\";
            DirectoryInfo diInfo = new DirectoryInfo(TargetFPath);
            int nListPos_Start = 0;
            int nListPos_End = 0;

            FileInfo[] fileInfo = diInfo.GetFiles("*.dsttesttmp");
            List<FileInfo> fileList = new List<FileInfo>();
            foreach (FileInfo f in fileInfo)
                fileList.Add(f);
            fileList.Sort(new FileComparer());
            int nFileIndex_Innder = 0;
            int nFileIndex_Dst = 0;
            bool bneedcmp = false;

            ucFileLostNum = ucCycleFileNum - fileList.Count;
            
            for (nFileIndex_Innder = 0; nFileIndex_Innder < refFI.FList.Count; nFileIndex_Innder++)
            {
                string fromFile = refFI.FList[nFileIndex_Innder];
                string strDstFullName = "";
                for (nFileIndex_Dst = 0; nFileIndex_Dst < fileList.Count; nFileIndex_Dst++)
                {
                    string strDstName_inner = fileList[nFileIndex_Dst].Name;
                    bneedcmp = IsNeedCompare(ref fromFile, ref strDstName_inner);
                    if (bneedcmp)
                    {
                        strDstFullName = fileList[nFileIndex_Dst].FullName;
                        FileStream fsource = new FileStream(fromFile, FileMode.Open);
                        FileStream fDst = new FileStream(strDstFullName, FileMode.Open);
                        long nSrcLength, nDstLength;
                        byte[] buf_src = new byte[1024];
                        byte[] buf_dst = new byte[1024];
                        int nCompareLength = 0;

                        //Compare the file in this area!!!Tomorrow finish
                        if (strDstName_inner == refFI.strWhichFileName)
                        {
                            //compare the last 1024 byte
                            if (!refCMPSetting.bEnableAll)
                            {
                                if (fDst.Length <= refCMPSetting.ulCmpSize)
                                {
                                    fsource.Seek(0, SeekOrigin.Begin);
                                    fDst.Seek(0, SeekOrigin.Begin);
                                    nCompareLength = (int)fDst.Length;
                                }
                                else
                                {
                                    fsource.Seek(refCMPSetting.ulCmpSize, SeekOrigin.Begin);
                                    fDst.Seek(refCMPSetting.ulCmpSize, SeekOrigin.Begin);
                                    nCompareLength = (int)refCMPSetting.ulCmpSize;
                                }
                                byte[] bytesrc = new byte[nCompareLength];
                                byte[] bytedst = new byte[nCompareLength];
                                fsource.Read(bytesrc, 0, nCompareLength);
                                fDst.Read(bytedst, 0, nCompareLength);
                                string outputS_inner;
                                string outputD_inner;
                                outputS_inner = System.Text.Encoding.ASCII.GetString(bytesrc);
                                outputD_inner = System.Text.Encoding.ASCII.GetString(bytedst);
                                fsource.Close();
                                fDst.Close();
                                if (outputS_inner != outputD_inner)
                                    this.ucFileFailCMP++;
                            }
                            else
                            {
                                long nleft = fDst.Length;
                                byte[] bytesrc ;
                                byte[] bytedst;
                                long nCMPSize = 0;
                                while (nleft >= 0)
                                {
                                    fDst.Seek(fDst.Length - nleft, SeekOrigin.Begin);
                                    fsource.Seek(fDst.Length - nleft, SeekOrigin.Begin);

                                    if (nleft >= 1024)
                                    {
                                        nCMPSize = 1024;
                                    }
                                    else
                                    {
                                        nCMPSize = nleft;
                                    }
                                    bytesrc = new byte[nCMPSize];
                                    bytedst = new  byte[nCMPSize];
                                    fsource.Read(bytesrc, 0, (int)nCMPSize);
                                    fDst.Read(bytedst, 0, (int)nCMPSize);
                                    string outputS;
                                    string outputD;
                                    outputS = System.Text.Encoding.ASCII.GetString(bytesrc);
                                    outputD = System.Text.Encoding.ASCII.GetString(bytedst);
                                    if (outputS != outputD)
                                    {
                                        Console.Write("compare file failed\n");
                                        this.ucFileFailCMP++;
                                        break;
                                        //return false;
                                    }
                                    nleft = nleft - nCMPSize;
                                }
                                fDst.Close();
                                fsource.Close();
                            }
                        }
                        else
                        {
                            //compare the file or the last 1024 byte
                            if (fsource.Length != fDst.Length)
                            {
                                Console.Write("compare file failed\n");
                                //return false;
                                this.ucFileFailCMP++;
                            }
                            else if (refCMPSetting.bEnableAll)
                            {
                                byte[] bufSrc = new byte[1024];
                                byte[] bufDst = new byte[1024];
                                long nCMPTimes = fsource.Length / 1024;
                                int nIndex = 0;
                                for (nIndex = 0; nIndex < (int)nCMPTimes; nIndex++)
                                {
                                    fsource.Seek(nIndex * 1024, SeekOrigin.Begin);
                                    fDst.Seek(nIndex * 1024, SeekOrigin.Begin);
                                    fDst.Read(bufDst, 0, 1024);
                                    fsource.Read(bufSrc, 0, 1024);
                                    string outputS;
                                    string outputD;
                                    outputS = System.Text.Encoding.ASCII.GetString(bufSrc);
                                    outputD = System.Text.Encoding.ASCII.GetString(bufDst);
                                    if (outputS != outputD)
                                    {
                                        Console.Write("compare file failed\n");
                                        this.ucFileFailCMP++;
                                        break;
                                        //return false;
                                    }
                                }
                            }
                            else
                            {
                                fsource.Seek(refCMPSetting.ulCmpSize, SeekOrigin.Begin);
                                fDst.Seek(refCMPSetting.ulCmpSize, SeekOrigin.Begin);

                                byte[] bufSrc = new byte[refCMPSetting.ulCmpSize];
                                byte[] bufDst = new byte[refCMPSetting.ulCmpSize];
                                fsource.Read(bufSrc, 0, (int)refCMPSetting.ulCmpSize);
                                fDst.Read(bufDst, 0, (int)refCMPSetting.ulCmpSize);
                                string outputS;
                                string outputD;
                                outputS = System.Text.Encoding.ASCII.GetString(bufSrc);
                                outputD = System.Text.Encoding.ASCII.GetString(bufDst);
                                if (outputS != outputD)
                                {
                                    Console.Write("compare file failed\n");
                                    this.ucFileFailCMP++;
                                    //return false;
                                }
                            }
                        }

                        fsource.Close();
                        fDst.Close();
                        fileList.RemoveAt(nFileIndex_Dst);
                        nFileIndex_Dst--;
                    }

                }
            }
        }
    

        public override void Delete_file(string TargetFPath, ref CFileItem refFI) {
            int nFileIndex = 0;
            if (TargetFPath[TargetFPath.Length - 1] != '\\') //非根目录
                TargetFPath += "\\";
            DirectoryInfo di = new DirectoryInfo(TargetFPath);
            foreach (FileInfo f in di.GetFiles("*.dsttesttmp"))
            {
                Console.Write(f.Name);
                f.Delete();
                
            }
        }
    }
}

namespace WindowsFormsApplication4
{
    class CSeniorTMethod : CMediumTMethod
    {
        private CCMPSeniorSetting refCMPSeniorSetting;
        public CSeniorTMethod()
        {
        }
        public CSeniorTMethod(int ucFileTimes_Arg,ref CCMPSetting refCMPSetting_Arg, ref CCMPSeniorSetting refCMPSeniorSetting_Arg)
        {
            ucFileLostNum = 0;
            ucFileTimes = ucFileTimes_Arg;
            
            ucFileFailCMP = 0;
            bDisk_PowerOff = false;
            ucRealFileNum = 0;
            refCMPSetting = refCMPSetting_Arg;
            refCMPSeniorSetting = refCMPSeniorSetting_Arg;
        }
        public override void Create_file(ref string strTargetPath, ref CSetting st)
        {
            if (strTargetPath[strTargetPath.Length - 1] != '\\') //非根目录
                strTargetPath += "\\";
            byte[] write_buf = new byte[32 * 1024];
            long write_size = refCMPSeniorSetting.ulSize * 1024 * 1024 * 1024;
            int nIndex = 0, nWriteIndex = 0;
            string strPath = "";
            for(nIndex = 0; nIndex < 32 * 1024; nIndex++)
            {
                write_buf[nIndex] = 0xAA;
            }
            for(nIndex = 0; nIndex < refCMPSeniorSetting.ulSize; nIndex++)
            {   
                strPath = nIndex.ToString() + ".rsvdata";
                strPath = strTargetPath + strPath;
                FileStream fs = new FileStream(strPath, FileMode.Append, FileAccess.Write);
                for(nWriteIndex = 0; nWriteIndex < 1024 * 1024 / 32; nWriteIndex++)
                {
                    fs.Write(write_buf, 0, 32 * 1024);
                    fs.Seek(0, SeekOrigin.End);
                }
                fs.Close();
            }
            base.Create_file(ref strTargetPath, ref st);
        }
        public override void Copy_file(string TargetFPath, ref CFileItem refFI)
        {
            base.Copy_file(TargetFPath, ref refFI);
        }
        public override void Compare_file(string TargetFPath, ref CFileItem refFI) {
            byte[] cmp_buf = new byte[1024];
            byte[] ffile_buf = new byte[1024];
            int ncmpIndex = 0;
            for (ncmpIndex = 0; ncmpIndex < 1024; ncmpIndex++)
                cmp_buf[ncmpIndex] = 0xAA;
            DirectoryInfo di = new DirectoryInfo(TargetFPath);
           
            if (refCMPSeniorSetting.bNeedCmp)
            {
                refCMPSeniorSetting.bNeedCmp = false;
                foreach(FileInfo fi in di.GetFiles("*.rsvdata"))
                {
                    string strPath = fi.FullName;
                    FileStream fs = new FileStream(strPath, FileMode.Open, FileAccess.Read);
                    int nCMPKSizeIndex = 0;
                    for(nCMPKSizeIndex = 0; nCMPKSizeIndex < (int)1024 * 1024; nCMPKSizeIndex++)
                    {
                        fs.Seek(nCMPKSizeIndex * 1024, SeekOrigin.Begin);
                        fs.Read(ffile_buf, 0, 1024);
                        string outputS;
                        string outputD;

                        outputS = System.Text.Encoding.ASCII.GetString(ffile_buf);
                        outputD = System.Text.Encoding.ASCII.GetString(cmp_buf);
                        if (outputD != outputS)
                        {
                            ucFileFailCMP_Senior++;
                            break;
                        }
                    }
                    fs.Close();
                }
            }
           
            base.Compare_file(TargetFPath, ref refFI);
           
        }
        public override void Delete_file(string TargetFPath, ref CFileItem refFI) {
            if (TargetFPath[TargetFPath.Length - 1] != '\\') //非根目录
                TargetFPath += "\\";
            DirectoryInfo di = new DirectoryInfo(TargetFPath);
            foreach (FileInfo f in di.GetFiles("*.rsvdata"))
            {
                Console.Write(f.Name);
                f.Delete();
            }
            base.Delete_file(TargetFPath, ref refFI);
        }
    }
}

