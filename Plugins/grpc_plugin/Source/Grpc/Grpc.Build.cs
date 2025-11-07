using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
//using System.Net.NetworkInformation;
//using System.Text;
using System.Text.RegularExpressions;
using UnrealBuildTool;

public class Grpc : ModuleRules
{
    class VcpkgBasePaths
    {
        public VcpkgBasePaths(string root)
        {
            this.root = root;
        }

        public string root;

        public string exe => Path.Combine(root, "vcpkg.exe");
        public string installed => Path.Combine(root, "installed");
        public string bootstrap => Path.Combine(root, "bootstrap-vcpkg.bat");

        public string triplets => Path.Combine(root, "triplets");
        public string tripletsCommunity => Path.Combine(triplets, "community");
    }

    class VcpkgPaths
    {
        public VcpkgPaths(string tripletName, VcpkgBasePaths basePaths)
        {
            this.tripletName = tripletName;
            this.basePaths = basePaths;
        }

        public VcpkgBasePaths basePaths;
        public string tripletName;

        public string info => Path.Combine(basePaths.installed, "vcpkg", "info");
        public string tripletInstalled => Path.Combine(basePaths.installed, tripletName);

        public string lib => Path.Combine(tripletInstalled, "lib");
        public string bin => Path.Combine(tripletInstalled, "bin");
        public string include => Path.Combine(tripletInstalled, "include");

        public string tools => Path.Combine(tripletInstalled, "tools");

        public bool isCommunity => File.Exists(possibletripletFile(basePaths.tripletsCommunity));
        public bool isOfficial => File.Exists(possibletripletFile(basePaths.triplets));

        public string tripletFile => possibletripletFile(isOfficial ? basePaths.triplets : basePaths.tripletsCommunity);
        

        //public string getTripletFile()
        //{

        //}

        private string possibletripletFile(string baseTripletPath) => Path.Combine(baseTripletPath/*basePaths.tripletsCommunity*/, tripletName + ".cmake");
    }

    //private string mPluginPath;
    //private string mCorePath;

    //private string mVcpkgPath;
    //private string mVcpkgExe;

    //private string mInfoPath;
    //private string mTripletPath;

    //private string mInstalledPath;
    //private string mLibPath;
    //private string mBinPath;
    //private string mIncludePath;
    
    //private string mTarget;
    private string mTargetTriplet;
    private string mHostTriplet;
    private string mProjectPath => Path.Combine(PluginDirectory, "..", "..");
    private string mProjectName
    {
        get
        {
            var Files = Directory.GetFiles(mProjectPath, "*.uproject");
            if (Files.Length != 1)
                throw new Exception("There is more than one project in the root folder");

            return Path.GetFileNameWithoutExtension(Files[0]);
        }
    }

    private static string runProgram(string program, string arguments, bool redirect = false)
    {
        ProcessStartInfo startInfo = new()
        {
            CreateNoWindow = false,
            UseShellExecute = false,
            FileName = program,
            Arguments = arguments,
            RedirectStandardOutput = redirect,
            RedirectStandardError = redirect,
        };

        string maybeOut = null;

        try
        {
            using (Process exeProcess = Process.Start(startInfo))
            {
                if (redirect)
                {
                    maybeOut = exeProcess.StandardOutput.ReadToEnd();
                    string err = exeProcess.StandardError.ReadToEnd();

                    maybeOut += "\n" + err;
                    maybeOut = maybeOut.Trim();
                }
                exeProcess.WaitForExit();

                return maybeOut;
            }
        }
        catch (Exception e)
        {
            throw new BuildException(e.Message);
        }
    }

    private HashSet<string> parseDepencies(VcpkgPaths vcpkgPaths, string package)
    {
        Regex reg = new Regex(@"^([^* :\[\]]*)(?:\[[^\]]*\])?:(.*)", RegexOptions.Multiline);
        HashSet<string> Packages = new HashSet<string>();
        
        string unparsed = runProgram(vcpkgPaths.basePaths.exe, /*"--overlay-ports=" + Path.GetFullPath(Path.Combine(PluginDirectory, "Source", "overlay")) + */" --vcpkg-root " + vcpkgPaths.basePaths.root + " depend-info " + package + ":" + vcpkgPaths.tripletName, true);
        //Console.WriteLine(unparsed);
        unparsed = unparsed.Replace(":" + vcpkgPaths.tripletName, "");

        MatchCollection MatchCollection = reg.Matches(unparsed);
        foreach (Match match in MatchCollection)
        {
            Packages.Add(match.Groups[1].Value);
            //Console.Write(match.Groups[1]);
            //Console.Write("    -_-    ");

            foreach (string SubPackage in match.Groups[2].Value.Split(','))
            {
                Packages.Add(SubPackage.Trim());
                //Console.Write($"\"{VARIABLE.Trim()}\"");
            }
            //Console.WriteLine("---------meh");
            /*for (int i = 1; i < match.Groups.Count; ++i)
            {
                */
                /*foreach (Capture capture in match.Groups[i].Captures)
                {
                    packages.Add(capture.Value);
                }*/
            /*}*/
        }
        Packages.RemoveWhere((input) => input.Contains("vcpkg"));
        Packages.Remove("");
        if (vcpkgPaths.tripletName.StartsWith("arm64-uwp"))
            Packages.Remove("c-ares");
        
        return Packages;
    }

    private List<string> getInstalledFiles(VcpkgPaths vcpkgPaths, IEnumerable<string> packages)
    {
        string[] files = Directory.GetFiles(vcpkgPaths.info);
        List<string> InstalledFiles = new List<string>();

        Console.WriteLine("Packages: [");
        foreach (string Package in packages)
        {
            Console.WriteLine(Package);
        }
        Console.WriteLine("]");

        foreach (string Package in packages)
        {
            Regex regex = new Regex("^" + Package + @"_[^_]*?_" + vcpkgPaths.tripletName + @"\.list$");

            bool found = false;
            foreach (string file in files)
            {
                if (!regex.IsMatch(Path.GetFileName(file))) continue;

                InstalledFiles.Add(file);
                found = true;
                break;
            }

            if (!found)
            {
                if (!Package.Contains("vcpkg")) 
                    throw new BuildException(Package + " is not installed");
            }
        }
        return InstalledFiles;
    }
    /*
    private static void linkFolder(string target, string src)
    {
        if (Directory.Exists(target)) return;
		Directory.CreateDirectory(Path.Combine(target, ".."));
        runProgram("cmd.exe", "/C mklink /J " + "\"" + target + "\"" + " " + "\"" + src + "\"");
        if (!Directory.Exists(target))
            throw new BuildException("Linking directory failed: " + src + " -> " + target);
    }
    */
    private static void checkInstalled(string path, Action install, bool precheck = true, bool postcheck = true)
    {
        if (precheck && File.Exists(path)) return;

        install();

        if (postcheck && !File.Exists(path))
            throw new BuildException("Failed at install of: " + Path.GetFileName(path));
    }

    class LibFiles
    {
        public LibFiles() {}

        public List<string> libs = new List<string>();
        public List<string> dlls = new List<string>();
    }

    private LibFiles getRessources(List<string> installedFiles, string triplet)
    {
        LibFiles result = new();

        Regex regLib = new Regex($@"^{triplet}\/lib\/.*?\.lib");
        Regex regBin = new Regex($@"^{triplet}\/bin\/.*?\.dll");

        //Regex regLib = new Regex("^" + triplet + @"\/lib\/.*?\.lib");
        //Regex regBin = new Regex("^" + triplet + @"\/bin\/.*?\.dll");

        foreach (string InstalledFile in installedFiles)
        {
            foreach (string Line in File.ReadAllLines(InstalledFile))
            {
                if (regLib.IsMatch(Line))
                    result.libs.Add(Path.GetFileName(Line));
                else if (regBin.IsMatch(Line))
                    result.dlls.Add(Path.GetFileName(Line));
            }
        }
        return result;
    }
    /*
    private void linkRessources()
    {
        linkFolder(mIncludePathLinked, mIncludePath);
        linkFolder(mLibPathLinked, mLibPath);
        if (mDlls.Count > 0)
            linkFolder(mBinPathLinked, mBinPath);
    }
    */
    private string getTarget(ReadOnlyTargetRules Target)
    {
        if (Target.Platform == UnrealTargetPlatform.Win64)
            return "x64-windows";
        else if (Target.Platform == UnrealTargetPlatform.HoloLens)
        {
            if (Target.Architecture == UnrealArch.Arm64)
                return "arm64-uwp";
            else
            {
                //TODO:: not supported
            }
        }

        return "";
    }

    private void generateProtoFiles(VcpkgPaths vcpkgPaths)
    {
        string gprc_plugin = Path.Combine(vcpkgPaths.tools, "grpc", "grpc_cpp_plugin.exe");
        string protoc = Path.Combine(vcpkgPaths.tools, "protobuf", "protoc.exe");
        string protoPath = Path.Combine(mProjectPath, "Proto");//Path.Combine(mCorePath, "assets", "grpc", "proto");

        string generatedDir = Path.Combine(mProjectPath, "Source", mProjectName, "Generated");

        //Console.WriteLine(Path.GetFullPath(generatedDir));

        Directory.CreateDirectory(generatedDir);
        foreach (FileInfo file in new DirectoryInfo(generatedDir).GetFiles()) 
            file.Delete();

        PublicIncludePaths.Add(generatedDir);

        runProgram(protoc, "--proto_path=" + protoPath + " --grpc_out=" + generatedDir + " --cpp_out=" + generatedDir + " --plugin=protoc-gen-grpc=" + gprc_plugin + " " + protoPath + "/*.proto");

        foreach (FileInfo file in new DirectoryInfo(generatedDir).GetFiles())
        {
            
            if (!file.FullName.EndsWith(".cc"))
            {
                //Console.WriteLine("Gen header: \t" + file.Name);
                //file.CopyTo(Path.Combine(outSource, file.Name), true);
                continue;
            }
            
            var tempFile = Path.GetTempFileName();
            var oldText = File.ReadAllLines(file.FullName).ToList();
            oldText.Insert(0, "#include \"grpc_include_begin.h\"");
            oldText.Add("#include \"grpc_include_end.h\"");

            File.WriteAllLines(tempFile, oldText);
            
            //Console.WriteLine("Gen cc: \t" + oldName);
            File.Move(tempFile, file.FullName, true);
        }
    }

    private string getHostTarget()
    {
        if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Win64)
            return "x64-windows";
        else if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.HoloLens)
        {
            return "arm64-uwp";
        }
        return "";

        //Console.WriteLine(BuildHostPlatform.Current.Platform);
    }

    private void makeReleaseOnly(VcpkgPaths paths)
    {
        if (File.ReadLines(paths.tripletFile).Contains("VCPKG_BUILD_TYPE release") && File.ReadLines(paths.tripletFile).Contains("VCPKG_PLATFORM_TOOLSET_VERSION \"14.38\""))
            return;

        var tempFile = Path.GetTempFileName();
        var linesToKeep = File.ReadLines(paths.tripletFile).Where(line => !line.Contains("VCPKG_BUILD_TYPE"));

        File.WriteAllLines(tempFile, linesToKeep);
        File.AppendAllLines(tempFile, new string[] { 
            "set(VCPKG_BUILD_TYPE release)",
            "set(VCPKG_PLATFORM_TOOLSET_VERSION \"14.38\")"
        });
        
        File.Move(tempFile, paths.tripletFile, true);
    }

    public Grpc(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDefinitions.Add("GOOGLE_PROTOBUF_NO_RTTI");
        PublicDefinitions.Add("GPR_FORBID_UNREACHABLE_CODE");
        PublicDefinitions.Add("GRPC_ALLOW_EXCEPTIONS=0");
	
	    //Console.WriteLine(PluginDirectory);
	    //Console.WriteLine(ModuleDirectory);

        //mCorePath = Path.GetFullPath(Path.Combine(mPluginPath, "..", "..", "..", ".."));
        VcpkgBasePaths basePaths = new VcpkgBasePaths(
            Path.GetFullPath(Path.Combine(PluginDirectory, "Source", "vcpkg"))
            );

        mTargetTriplet = getTarget(Target) + "-static-md";
        mHostTriplet = getHostTarget() + "-static-md";

        VcpkgPaths HostPaths = new VcpkgPaths(mHostTriplet, basePaths);
        VcpkgPaths TargetPaths = new VcpkgPaths(mTargetTriplet, basePaths);
        
        List<string> Packages = new List<string>();
        Packages.AddRange(new string[] { "grpc", /*"draco",*/ "asio-grpc" });

        checkInstalled(basePaths.exe, () =>
        {
            Console.WriteLine("Bootstrapping vcpkg");
            runProgram(basePaths.bootstrap, "-disableMetrics");
        }, false);

        string VcpkgCmd0 = "install --recurse"/* --overlay-ports=" + Path.GetFullPath(Path.Combine(PluginDirectory, "Source", "overlay")) */+ " --host-triplet=" + mHostTriplet + " --vcpkg-root " + basePaths.root + " vcpkg-cmake";
        runProgram(basePaths.exe, VcpkgCmd0);

        makeReleaseOnly(HostPaths);
        if (mHostTriplet != mTargetTriplet)
            makeReleaseOnly(TargetPaths);

        string VcpkgCmd = "install --recurse"/* --overlay-ports=" + Path.GetFullPath(Path.Combine(PluginDirectory, "Source", "overlay")) */+ " --host-triplet=" + mHostTriplet + " --vcpkg-root " + basePaths.root;
        string InstallMessage = "Installing [";
        HashSet<string> SubPackages = new HashSet<string>();
        foreach (var Package in Packages)
        {
            SubPackages.UnionWith(parseDepencies(TargetPaths, Package));
            VcpkgCmd += " " + Package + ":" + mTargetTriplet;
            InstallMessage += " " + Package;
        }

        Console.Write("Necessary packages: [");
        foreach (var Package in SubPackages)
            Console.Write(Package + " ");
        Console.WriteLine("]");

        InstallMessage += " ] for " + mTargetTriplet + " with host " + mHostTriplet;

        Console.WriteLine(InstallMessage);
        runProgram(basePaths.exe, VcpkgCmd);

        //foreach (string Package in SubPackages)
          //  Console.WriteLine(Package);

        List<string> InstalledFiles = getInstalledFiles(TargetPaths, SubPackages);
        var Ressources = getRessources(InstalledFiles, mTargetTriplet);

        Console.WriteLine();
        foreach (string InstalledFile in InstalledFiles)
            Console.WriteLine(Path.GetFileNameWithoutExtension(InstalledFile).Replace("_" + TargetPaths.tripletName, "") + " is installed");

        Console.WriteLine("Added header root: " + TargetPaths.include);
        PublicIncludePaths.Add(TargetPaths.include);
        //string[] allSubDirectories = Directory.GetDirectories(TargetPaths.include, "*", SearchOption.AllDirectories);
        //foreach (string subDirectory in allSubDirectories)
          //  PublicIncludePaths.Add(subDirectory);

        {
            int i = 0;
            Console.WriteLine("Adding lib files: ");
            foreach (string Lib in Ressources.libs)
            {
                if (i % 10 == 0)
                    Console.Write("[ ");

                Console.Write(Lib.Replace(".lib", "") + " ");

                if (++i % 10 == 0)
                    Console.WriteLine("]");

                string LibFilePath = Path.Combine(TargetPaths.lib, Lib);
                //Console.Write("\n" + Path.GetFileNameWithoutExtension(LibFilePath) + " ");
                PublicAdditionalLibraries.Add(LibFilePath);
            }
            if (i % 10 != 0)
                Console.WriteLine("]");
        }
        
        Console.WriteLine("Dlls:");
        foreach (string Dll in Ressources.dlls)
        {
            string DllFilePath = Path.Combine(TargetPaths.bin, Dll);
            Console.Write("\n" + Path.GetFileNameWithoutExtension(DllFilePath) + " ");
            RuntimeDependencies.Add("$(TargetOutputDir)/" + Dll, DllFilePath);
        }
        //Console.WriteLine();

        string ProtoDir = Path.Combine(mProjectPath, "Proto");
        if (Directory.Exists(ProtoDir))
            generateProtoFiles(HostPaths);

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        PublicSystemLibraries.Add("crypt32.lib");

        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine" });
    }
}
