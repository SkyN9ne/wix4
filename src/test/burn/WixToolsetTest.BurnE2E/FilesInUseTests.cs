// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

namespace WixToolsetTest.BurnE2E
{
    using System.IO;
    using WixTestTools;
    using Xunit.Abstractions;

    public class FilesInUseTests : BurnE2ETests
    {
        public FilesInUseTests(ITestOutputHelper testOutputHelper) : base(testOutputHelper) { }

        [RuntimeFact]
        public void CanCancelInstallAfterRetryingLockedFile()
        {
            var packageA = this.CreatePackageInstaller("PackageA");
            var bundleA = this.CreateBundleInstaller("BundleA");
            var testBAController = this.CreateTestBAController();

            testBAController.SetPackageRetryExecuteFilesInUse("PackageA", 1);

            packageA.VerifyInstalled(false);

            // Lock the file that will be installed.
            string targetInstallFile = packageA.GetInstalledFilePath("Package.wxs");
            Directory.CreateDirectory(Path.GetDirectoryName(targetInstallFile));
            using (FileStream lockTargetFile = new FileStream(targetInstallFile, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.None, 4096, FileOptions.DeleteOnClose))
            {
                bundleA.Install(expectedExitCode: (int)MSIExec.MSIExecReturnCode.ERROR_INSTALL_USEREXIT);
            }

            bundleA.VerifyUnregisteredAndRemovedFromPackageCache();

            packageA.VerifyInstalled(false);
        }

        [RuntimeFact]
        public void WixStdBAFailsWithLockedFile()
        {
            var packageA = this.CreatePackageInstaller("PackageA");
            var bundleA = this.CreateBundleInstaller("WixStdBaBundle");

            packageA.VerifyInstalled(false);

            bundleA.Install();

            packageA.VerifyInstalled(true);

            // Lock the file that will be uninstalled.
            var targetInstallFile = packageA.GetInstalledFilePath("Package.wxs");
            using (var lockTargetFile = new FileStream(targetInstallFile, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
            {
                bundleA.Uninstall(expectedExitCode: (int)MSIExec.MSIExecReturnCode.ERROR_INSTALL_FAILURE);
            }
        }
    }
}
