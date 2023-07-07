//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                          Copyright 2007-2020 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Windows.Forms;


namespace WinRing0SampleCs
{
    class Program
    {
        /// <summary>
        /// Main Entry Point
        /// </summary>
        [STAThread]
        public static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new WinRing0Sample());
        }
    }
}