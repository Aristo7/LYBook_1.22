/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            struct ExportProduct
            {
                static const s8 s_LodNotUsed = -1;

                SCENE_CORE_API ExportProduct(const AZStd::string& filename, Uuid id, Data::AssetType assetType, s8 lod);
                SCENE_CORE_API ExportProduct(AZStd::string&& filename, Uuid id, Data::AssetType assetType, s8 lod);
                ExportProduct(const ExportProduct& rhs) = default;
                SCENE_CORE_API ExportProduct(ExportProduct&& rhs);

                ExportProduct& operator=(const ExportProduct& rhs) = default;
                SCENE_CORE_API ExportProduct& operator=(ExportProduct&& rhs);

                //! Other names the product file may be known as in the past. This is only backwards compatibility in ResourceCompilerScene.
                AZStd::vector<AZStd::string> m_legacyFileNames;
                //! Relative or absolute path of the product file.
                AZStd::string m_filename;
                //! Unique id for the product file. This is usually based on the group id and is used to generate the 
                //! the sub id.
                Uuid m_id;
                //! Type of the product file.
                Data::AssetType m_assetType;
                //! If the product makes use of level of detail, the level is encoded in the sub id. If s_LodNotUsed is specified
                //! (default) the entire sub id number will be used for the product id.
                s8 m_lod;
                //! Relative path dependencies for autogenerated FBX materials
                AZStd::vector<AZStd::string> m_legacyPathDependencies;
                //! In the case of CGFs, we will have LOD export products that are dependencies of the base LOD
                AZStd::vector<ExportProduct> m_productDependencies;
            };

            class ExportProductList
            {
            public:
                SCENE_CORE_API ExportProduct& AddProduct(const AZStd::string& filename, Uuid id, Data::AssetType assetType, s8 lod = ExportProduct::s_LodNotUsed);
                SCENE_CORE_API ExportProduct& AddProduct(AZStd::string&& filename, Uuid id, Data::AssetType assetType, s8 lod = ExportProduct::s_LodNotUsed);

                SCENE_CORE_API const AZStd::vector<ExportProduct>& GetProducts() const;

                SCENE_CORE_API void AddDependencyToProduct(const AZStd::string& productName, ExportProduct& dependency);

            private:
                AZStd::vector<ExportProduct> m_products;
            };
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ