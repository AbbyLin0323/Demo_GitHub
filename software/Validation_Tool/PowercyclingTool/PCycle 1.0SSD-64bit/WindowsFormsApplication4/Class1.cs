using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WindowsFormsApplication4
{
    public class CFileItem
    {
        public List<string> FList;
        public long uFileSeekPointer;
        public int uWhichFile;
        public string strWhichFileName;
        public uint uCycle;
        public uint nHowManyTimeSingleFile;
        public CFileItem()
        {
            FList = new List<string>();
            uCycle = 0;
            ClearAllInfo();
        }
        public void ClearAllInfo()
        {
            uFileSeekPointer = 0;
            uWhichFile = 0;
            nHowManyTimeSingleFile = 0;
            strWhichFileName = "";
        }
    }
    public class CSetting
    {
        public long size_big;
        public long size_medium;
        public long size_small;

        public int num_big;
        public int num_medium;
        public int num_small;

        public int value_big;
        public int value_medium;
        public int value_small;

        public bool bEnableBigFile;
        public bool bEnableMediumFile;
        public bool bEnableSmallFile;

        public CSetting()
        {
            size_big = 1;
            size_medium = 200;
            size_small = 15;

            num_big = 1;
            num_medium = 5;
            num_small = 10;

            value_big = 0;
            value_medium = 0;
            value_small = 0;

            bEnableBigFile = false;
            bEnableMediumFile = false;
            bEnableSmallFile = false;
        }
    }
    public class CCMPSeniorSetting
    {
        public bool bNeedCmp;
        public long ulSize;
        public CCMPSeniorSetting()
        {
            bNeedCmp = false;
            ulSize = 90;
        }
    }

    public class CCMPSetting
    {
        public long ulCmpSize;
        public bool bEnableAll;

        public CCMPSetting()
        {
            bEnableAll = false;
            ulCmpSize = 12;
        }
    }
}
