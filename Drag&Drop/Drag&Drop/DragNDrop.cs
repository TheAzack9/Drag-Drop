// Uncomment these only if you want to export GetString() or ExecuteBang().
#define DLLEXPORT_GETSTRING
#define DLLEXPORT_EXECUTEBANG

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Rainmeter;
using System.Text;
using Microsoft.VisualBasic.FileIO;
using System.IO;
using vbAccelerator.Components.Shell;

// Overview: This is a blank canvas on which to build your plugin.

// Note: Measure.GetString, Plugin.GetString, Measure.ExecuteBang, and
// Plugin.ExecuteBang have been commented out. If you need GetString
// and/or ExecuteBang and you have read what they are used for from the
// SDK docs, uncomment the function(s). Otherwise leave them commented out
// (or get rid of them)!

namespace DragNDrop
{
    //Pinvoke methods used to allow the rainmeter skin to accept drag and drop actions
    class NativeMethods
    {
        [DllImport("shell32.dll")]
        internal static extern void DragAcceptFiles(IntPtr hwnd, bool fAccept);
        // Constant value was found in the "windows.h" header file.
        internal const int WM_DROPFILES = 0x233;


        [DllImport("shell32.dll")]
        internal static extern uint DragQueryFile(IntPtr hDrop, uint iFile, [Out] StringBuilder filename, uint cch);

        [DllImport("shell32.dll")]
        internal static extern void DragFinish(IntPtr hDrop);

    }
    internal class Measure
    {
        //Skin window handle
        private IntPtr skinHWND;
        //Custom class using NativeWindow that lets me hook into the skins HWNDPROC, which lets me detect system events :D
        RainmeterWindowWrapper wrapper;

        //The rest is basically a mess, but hey... it works :D
        internal Measure(API api)
        {
            Skin = api.GetSkin();
            skinHWND = api.GetSkinWindow();
            wrapper = new RainmeterWindowWrapper(skinHWND);
            wrapper.Callback = (m) =>
            {
                switch(m.Msg)
                {
                    case NativeMethods.WM_DROPFILES:
                        HandleDropFiles(m.WParam);
                        break;
                }

            };
            NativeMethods.DragAcceptFiles(skinHWND, true);
        }
        private void HandleDropFiles(IntPtr hDrop)
        {

            const int MAX_PATH = 260;

            var count = NativeMethods.DragQueryFile(hDrop, 0xFFFFFFFF, null, 0);

            for (uint i = 0; i < count; i++)
            {
                int size = (int)NativeMethods.DragQueryFile(hDrop, i, null, 0);

                var filename = new StringBuilder(size + 1);
                NativeMethods.DragQueryFile(hDrop, i, filename, MAX_PATH);

                HandleFileDrop(filename.ToString());

            }

            NativeMethods.DragFinish(hDrop);
            
        }

        internal enum Action
        {
            Shortcut,
            Copy,
            Delete,
            Move,
            Path

        }
        Action mAction = Action.Copy;
        internal string lastPath = "";
        internal string OnDroppedAction = "";
        internal string[] OnDropActionSubstitute;
        internal IntPtr Skin;
        internal string EndPath = "";
        internal bool Disabled = false;

        private bool DirectoryExists()
        {
            if (EndPath == "")
            {
                API.Log(API.LogType.Error, "Path not set for Drag&Drop measure!");
                return false;
            }
            if (!Directory.Exists(EndPath))
                Directory.CreateDirectory(EndPath);
            return true;
        }

        private void HandleFileDrop(string file)
        {
            if (Disabled)
                return;
            FileAttributes attr = System.IO.File.GetAttributes(file);
            
            switch (mAction)
            {
                case Action.Path:
                    lastPath = file;
                    break;

                case Action.Copy:
                    if (!DirectoryExists())
                        return;
                    if ((attr & FileAttributes.Directory) == FileAttributes.Directory)
                        FileSystem.CopyDirectory(file, EndPath + "\\" + Path.GetFileName(file));
                    else
                        System.IO.File.Copy(file, EndPath + "\\" + Path.GetFileName(file));
                    break;

                case Action.Delete:
                    if ((attr & FileAttributes.Directory) == FileAttributes.Directory)
                        FileSystem.DeleteDirectory(file, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin);
                    else
                        FileSystem.DeleteFile(file, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin);
                    break;

                case Action.Move:
                    if (!DirectoryExists())
                        return;

                    if ((attr & FileAttributes.Directory) == FileAttributes.Directory)
                        Directory.Move(file, EndPath + "\\" + Path.GetFileName(file));
                    else
                        System.IO.File.Move(file, EndPath + "\\" + Path.GetFileName(file));
                    break;

                case Action.Shortcut:
                    if (!DirectoryExists())
                        return;
                    API.Log(API.LogType.Debug, EndPath + "\\" + Path.GetFileNameWithoutExtension(file));
                    using (ShellLink shortcut = new ShellLink())
                    {

                        shortcut.Target = file;
                        shortcut.ShortCutFile = EndPath + "\\" + Path.GetFileNameWithoutExtension(file) + ".lnk";
                        shortcut.Description = "Automatically generated Shortcut using the Drag&Drop plugin for Rainmeter!";
                        shortcut.DisplayMode = ShellLink.LinkDisplayMode.edmNormal;
                        shortcut.Save();
                    }
                    break;
            }
            string bang = OnDroppedAction.Replace("$FileName$", Path.GetFileNameWithoutExtension(file)).Replace("$FileType$", Path.GetExtension(file)).Replace("$FileDirectory$", Path.GetDirectoryName(file));
            foreach(var sub in OnDropActionSubstitute)
            {
                var pair = sub.Split(':');
                if (pair.Length != 2)
                {
                    API.Log(API.LogType.Error, "Found an invalid substitute in OnDropActionSubstitute: " + sub);
                }
                bang = bang.Replace(pair[0], pair[1]);
            }
            API.Execute(Skin, bang);
        }

        internal void Reload(Rainmeter.API api, ref double maxValue)
        {
            string action = api.ReadString("Action", "");
            OnDroppedAction = api.ReadString("OnDroppedAction", "");
            OnDropActionSubstitute = api.ReadString("OnDropActionSubstitute", "").Split(',');
            EndPath = api.ReadString("Path", "");
            Disabled = api.ReadInt("Disabled", 0) != 0;
            if(action != "")
            try
            {
                mAction = (Action)Enum.Parse(typeof(Action), action);
            }
            catch
            {
                API.Log(API.LogType.Error, "Drag&Drop: Invalid Action type: " + action);
            }
            
                
        }

        internal double Update()
        {

            return 0.0;
        }

        internal void Dispose()
        {
            wrapper.Release();
            NativeMethods.DragAcceptFiles(skinHWND, false);
        }

#if DLLEXPORT_GETSTRING
        internal string GetString()
        {
            if (Disabled)
                return "";
            if(mAction == Action.Path)
            return lastPath;
            return "";
        }
#endif

#if DLLEXPORT_EXECUTEBANG
        internal void ExecuteBang(string args)
        {
        }
#endif
    }

    public static class Plugin
    {
#if DLLEXPORT_GETSTRING
        static IntPtr StringBuffer = IntPtr.Zero;
#endif

        [DllExport]
        public static void Initialize(ref IntPtr data, IntPtr rm)
        {
            data = GCHandle.ToIntPtr(GCHandle.Alloc(new Measure(new Rainmeter.API(rm))));
        }

        [DllExport]
        public static void Finalize(IntPtr data)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            measure.Dispose();
            GCHandle.FromIntPtr(data).Free();
            
#if DLLEXPORT_GETSTRING
            if (StringBuffer != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(StringBuffer);
                StringBuffer = IntPtr.Zero;
            }
#endif
        }

        [DllExport]
        public static void Reload(IntPtr data, IntPtr rm, ref double maxValue)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            measure.Reload(new Rainmeter.API(rm), ref maxValue);
        }

        [DllExport]
        public static double Update(IntPtr data)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            return measure.Update();
        }
        
#if DLLEXPORT_GETSTRING
        [DllExport]
        public static IntPtr GetString(IntPtr data)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            if (StringBuffer != IntPtr.Zero)
            {
                Marshal.FreeHGlobal(StringBuffer);
                StringBuffer = IntPtr.Zero;
            }

            string stringValue = measure.GetString();
            if (stringValue != null)
            {
                StringBuffer = Marshal.StringToHGlobalUni(stringValue);
            }

            return StringBuffer;
        }
#endif

#if DLLEXPORT_EXECUTEBANG
        [DllExport]
        public static void ExecuteBang(IntPtr data, IntPtr args)
        {
            Measure measure = (Measure)GCHandle.FromIntPtr(data).Target;
            measure.ExecuteBang(Marshal.PtrToStringUni(args));
        }
#endif
    }
}
