// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

namespace WixToolset.Core.WindowsInstaller
{
    using System;
    using System.Collections.Generic;
    using WixToolset.Core.WindowsInstaller.Bind;
    using WixToolset.Data.WindowsInstaller;
    using WixToolset.Extensibility;
    using WixToolset.Extensibility.Data;
    using WixToolset.Extensibility.Services;

    internal class MspBackend : IBackend
    {
        public IBindResult Bind(IBindContext context)
        {
            var messaging = context.ServiceProvider.GetService<IMessaging>();

            var backendHelper = context.ServiceProvider.GetService<IBackendHelper>();

            var extensionManager = context.ServiceProvider.GetService<IExtensionManager>();

            var backendExtensions = extensionManager.GetServices<IWindowsInstallerBackendBinderExtension>();

            foreach (var extension in backendExtensions)
            {
                extension.PreBackendBind(context);
            }

            // Create transforms named in patch transforms.
            IEnumerable<PatchTransform> patchTransforms;
            {
                var command = new CreatePatchTransformsCommand(messaging, backendHelper, context.IntermediateRepresentation, context.IntermediateFolder);
                patchTransforms = command.Execute();
            }

            // Enhance the intermediate by attaching the created patch transforms.
            IEnumerable<SubStorage> subStorages;
            {
                var command = new AttachPatchTransformsCommand(messaging, backendHelper, context.IntermediateRepresentation, patchTransforms);
                subStorages = command.Execute();
            }

            // Create WindowsInstallerData with patch metdata and transforms as sub-storages
            // Create MSP from WindowsInstallerData
            IBindResult result = null;
            try
            {
                var command = new BindDatabaseCommand(context, backendExtensions, subStorages);
                result = command.Execute();

                foreach (var extension in backendExtensions)
                {
                    extension.PostBackendBind(result);
                }

                return result;
            }
            catch
            {
                result?.Dispose();
                throw;
            }
        }

#if TODO_PATCHING
        public Intermediate Unbind(IUnbindContext context)
        {
            Output patch;

            // patch files are essentially database files (use a special flag to let the API know its a patch file)
            try
            {
                using (Database database = new Database(context.InputFilePath, OpenDatabase.ReadOnly | OpenDatabase.OpenPatchFile))
                {
                    var unbindCommand = new UnbindDatabaseCommand(context.Messaging, database, context.InputFilePath, OutputType.Patch, context.ExportBasePath, context.IntermediateFolder, context.IsAdminImage, context.SuppressDemodularization, skipSummaryInfo: false);
                    patch = unbindCommand.Execute();
                }
            }
            catch (Win32Exception e)
            {
                if (0x6E == e.NativeErrorCode) // ERROR_OPEN_FAILED
                {
                    throw new WixException(WixErrors.OpenDatabaseFailed(context.InputFilePath));
                }

                throw;
            }

            // retrieve the transforms (they are in substorages)
            using (Storage storage = Storage.Open(context.InputFilePath, StorageMode.Read | StorageMode.ShareDenyWrite))
            {
                Table summaryInformationTable = patch.Tables["_SummaryInformation"];
                foreach (Row row in summaryInformationTable.Rows)
                {
                    if (8 == (int)row[0]) // PID_LASTAUTHOR
                    {
                        string value = (string)row[1];

                        foreach (string decoratedSubStorageName in value.Split(';'))
                        {
                            string subStorageName = decoratedSubStorageName.Substring(1);
                            string transformFile = Path.Combine(context.IntermediateFolder, String.Concat("Transform", Path.DirectorySeparatorChar, subStorageName, ".mst"));

                            // ensure the parent directory exists
                            Directory.CreateDirectory(Path.GetDirectoryName(transformFile));

                            // copy the substorage to a new storage for the transform file
                            using (Storage subStorage = storage.OpenStorage(subStorageName))
                            {
                                using (Storage transformStorage = Storage.CreateDocFile(transformFile, StorageMode.ReadWrite | StorageMode.ShareExclusive | StorageMode.Create))
                                {
                                    subStorage.CopyTo(transformStorage);
                                }
                            }

                            // unbind the transform
                            var unbindCommand= new UnbindTransformCommand(context.Messaging, transformFile, (null == context.ExportBasePath ? null : Path.Combine(context.ExportBasePath, subStorageName)), context.IntermediateFolder);
                            var transform = unbindCommand.Execute();

                            patch.SubStorages.Add(new SubStorage(subStorageName, transform));
                        }

                        break;
                    }
                }
            }

            // extract the files from the cabinets
            // TODO: use per-transform export paths for support of multi-product patches
            if (null != context.ExportBasePath && !context.SuppressExtractCabinets)
            {
                using (Database database = new Database(context.InputFilePath, OpenDatabase.ReadOnly | OpenDatabase.OpenPatchFile))
                {
                    foreach (SubStorage subStorage in patch.SubStorages)
                    {
                        // only patch transforms should carry files
                        if (subStorage.Name.StartsWith("#", StringComparison.Ordinal))
                        {
                            var extractCommand = new ExtractCabinetsCommand(subStorage.Data, database, context.InputFilePath, context.ExportBasePath, context.IntermediateFolder);
                            extractCommand.Execute();
                        }
                    }
                }
            }

            return patch;
        }
#endif
    }
}
