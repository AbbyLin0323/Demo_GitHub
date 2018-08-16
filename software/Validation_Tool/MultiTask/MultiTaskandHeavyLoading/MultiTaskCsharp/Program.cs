using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace MultiTaskCsharp
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]

      
        public static void Main()
        {
            //Form1 form1 = new Form1();
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
