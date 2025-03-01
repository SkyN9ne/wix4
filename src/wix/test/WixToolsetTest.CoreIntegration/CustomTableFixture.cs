// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

namespace WixToolsetTest.CoreIntegration
{
    using System.IO;
    using System.Xml.Linq;
    using WixBuildTools.TestSupport;
    using WixToolset.Core.TestPackage;
    using Xunit;

    public class CustomTableFixture
    {
        [Fact]
        public void PopulatesCustomTable1()
        {
            var folder = TestData.Get(@"TestData");

            using (var fs = new DisposableFileSystem())
            {
                var baseFolder = fs.GetFolder();
                var intermediateFolder = Path.Combine(baseFolder, "obj");
                var msiPath = Path.Combine(baseFolder, @"bin\test.msi");

                var result = WixRunner.Execute(new[]
                {
                    "build",
                    Path.Combine(folder, "CustomTable", "CustomTable.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "MinimalComponentGroup.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "Product.wxs"),
                    "-bindpath", Path.Combine(folder, "SingleFile", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", msiPath
                });

                result.AssertSuccess();

                Assert.True(File.Exists(msiPath));
                var results = Query.QueryDatabase(msiPath, new[] { "CustomTable1" });
                WixAssert.CompareLineByLine(new[]
                {
                    "CustomTable1:Row1\ttest.txt",
                    "CustomTable1:Row2\ttest.txt",
                }, results);
            }
        }

        [Fact]
        public void PopulatesCustomTableWithLocalization()
        {
            var folder = TestData.Get(@"TestData");

            using (var fs = new DisposableFileSystem())
            {
                var baseFolder = fs.GetFolder();
                var intermediateFolder = Path.Combine(baseFolder, "obj");
                var msiPath = Path.Combine(baseFolder, @"bin\test.msi");

                var result = WixRunner.Execute(new[]
                {
                    "build",
                    Path.Combine(folder, "CustomTable", "LocalizedCustomTable.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "MinimalComponentGroup.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "Product.wxs"),
                    "-loc", Path.Combine(folder, "CustomTable", "LocalizedCustomTable.en-us.wxl"),
                    "-bindpath", Path.Combine(folder, "SingleFile", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", msiPath
                });

                result.AssertSuccess();

                Assert.True(File.Exists(msiPath));
                var results = Query.QueryDatabase(msiPath, new[] { "CustomTableLocalized" });
                WixAssert.CompareLineByLine(new[]
                {
                    "CustomTableLocalized:Row1\tThis is row one",
                    "CustomTableLocalized:Row2\tThis is row two",
                }, results);
            }
        }

        [Fact]
        public void PopulatesCustomTableWithFilePath()
        {
            var folder = TestData.Get(@"TestData");

            using (var fs = new DisposableFileSystem())
            {
                var baseFolder = fs.GetFolder();
                var intermediateFolder = Path.Combine(baseFolder, "obj");
                var msiPath = Path.Combine(baseFolder, @"bin\test.msi");

                var result = WixRunner.Execute(new[]
                {
                    "build",
                    Path.Combine(folder, "CustomTable", "CustomTableWithFile.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "MinimalComponentGroup.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "Product.wxs"),
                    "-bindpath", Path.Combine(folder, "CustomTable", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", msiPath
                });

                result.AssertSuccess();

                Assert.True(File.Exists(msiPath));
                var results = Query.QueryDatabase(msiPath, new[] { "CustomTableWithFile" });
                WixAssert.CompareLineByLine(new[]
                {
                    "CustomTableWithFile:Row1\t[Binary data]",
                    "CustomTableWithFile:Row2\t[Binary data]",
                }, results);
            }
        }

        [Fact]
        public void PopulatesCustomTableWithFilePathSerialized()
        {
            var folder = TestData.Get(@"TestData");

            using (var fs = new DisposableFileSystem())
            {
                var baseFolder = fs.GetFolder();
                var intermediateFolder = Path.Combine(baseFolder, "obj");
                var wixlibPath = Path.Combine(baseFolder, @"bin\test.wixlib");
                var msiPath = Path.Combine(baseFolder, @"bin\test.msi");

                var result = WixRunner.Execute(new[]
                {
                    "build",
                    Path.Combine(folder, "CustomTable", "CustomTableWithFile.wxs"),
                    "-bindpath", Path.Combine(folder, "CustomTable", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", wixlibPath
                });

                result.AssertSuccess();

                result = WixRunner.Execute(new[]
                {
                    "build",
                    Path.Combine(folder, "ProductWithComponentGroupRef", "MinimalComponentGroup.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "Product.wxs"),
                    "-lib", wixlibPath,
                    "-bindpath", Path.Combine(folder, "CustomTable", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", msiPath
                });

                result.AssertSuccess();

                Assert.True(File.Exists(msiPath));
                var results = Query.QueryDatabase(msiPath, new[] { "CustomTableWithFile" });
                WixAssert.CompareLineByLine(new[]
                {
                    "CustomTableWithFile:Row1\t[Binary data]",
                    "CustomTableWithFile:Row2\t[Binary data]",
                }, results);
            }
        }

        [Fact]
        public void UnrealCustomTableIsNotPresentInMsi()
        {
            var folder = TestData.Get(@"TestData");

            using (var fs = new DisposableFileSystem())
            {
                var baseFolder = fs.GetFolder();
                var intermediateFolder = Path.Combine(baseFolder, "obj");
                var msiPath = Path.Combine(baseFolder, @"bin\test.msi");

                var result = WixRunner.Execute(new[]
                {
                    "build",
                    Path.Combine(folder, "CustomTable", "CustomTable.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "MinimalComponentGroup.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "Product.wxs"),
                    "-bindpath", Path.Combine(folder, "SingleFile", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", msiPath
                });

                result.AssertSuccess();

                Assert.True(File.Exists(msiPath));
                var results = Query.QueryDatabase(msiPath, new[] { "CustomTable2" });
                WixAssert.StringCollectionEmpty(results);
            }
        }

        [Fact]
        public void CanCompileAndDecompile()
        {
            var folder = TestData.Get(@"TestData");
            var expectedFile = Path.Combine(folder, "CustomTable", "CustomTable-Expected.wxs");

            using (var fs = new DisposableFileSystem())
            {
                var baseFolder = fs.GetFolder();
                var intermediateFolder = Path.Combine(baseFolder, "obj");
                var msiPath = Path.Combine(baseFolder, @"bin\test.msi");
                var decompiledWxsPath = Path.Combine(baseFolder, @"decompiled.wxs");

                var result = WixRunner.Execute(new[]
                {
                    "build",
                    "-d", "ProductCode=83f9c623-26fe-42ab-951e-170022117f54",
                    Path.Combine(folder, "CustomTable", "CustomTable.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "MinimalComponentGroup.wxs"),
                    Path.Combine(folder, "ProductWithComponentGroupRef", "Product.wxs"),
                    "-bindpath", Path.Combine(folder, "SingleFile", "data"),
                    "-intermediateFolder", intermediateFolder,
                    "-o", msiPath
                });

                result.AssertSuccess();
                Assert.True(File.Exists(msiPath));

                result = WixRunner.Execute(new[]
                {
                    "msi", "decompile", msiPath,
                    "-sw1060",
                    "-intermediateFolder", intermediateFolder,
                    "-o", decompiledWxsPath
                });

                result.AssertSuccess();

                WixAssert.CompareXml(expectedFile, decompiledWxsPath);
            }
        }
    }
}
