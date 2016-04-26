using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace DragNDrop
{
    class RainmeterWindowWrapper : NativeWindow
    {

        
        private IntPtr window;
        public Action<Message> Callback = null;


        public RainmeterWindowWrapper(IntPtr window)
        {
            this.window = window;
            //When plugin is loaded, create hook
            AssignHandle(window);
        }


        internal void Release()
        {
            // Plugin is disposed, release hook.
            ReleaseHandle();
        }

        [System.Security.Permissions.PermissionSet(System.Security.Permissions.SecurityAction.Demand, Name = "FullTrust")]
        protected override void WndProc(ref Message m)
        {
            // Listen for operating system messages

            if (Callback != null)
                Callback(m);

            base.WndProc(ref m);
        }
       
    }
}
