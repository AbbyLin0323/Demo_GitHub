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

namespace MultiTaskCsharp
{
    public partial class Form9 : Form
    {
        public TextWriter output_T8;
        public Form1 form1;

         public Form9(Form1 form1)
        {
            InitializeComponent();
            this.form1 = form1;
            output_T8 = TextWriter.Synchronized(new TextBoxWriter(T8_Information));
            this.Closing += new System.ComponentModel.CancelEventHandler(Form9_Closing);

        }
        [DllImport("kernel32.dll", BestFitMapping = true, CharSet = CharSet.Ansi)]
        public static extern bool CloseHandle
            (
               IntPtr hobject
            );

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


        //Form1 form1 = new Form1();
        private void Form9_Closing(object sender, CancelEventArgs e)
        {
            form1.Task[7].Abort();
            CloseHandle(form1.File_dest[7]);
            CloseHandle(form1.File_src[7]);
            //form1.Delete_File((Byte)8, (Byte)1);

        }         
        
           
       
      
        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            //Console.WriteLine("aa");
            // Form1 form1 = new Form1();
            //this.textBox1.Text = form1.taskkk.destDisk;
         
            
        }
    }
}
