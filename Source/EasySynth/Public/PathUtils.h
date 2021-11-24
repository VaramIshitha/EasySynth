// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"


class FPathUtils
{
public:
	/**
	 * Universal utils
	*/

	/** Plugin name to be referenced by other methods */
	static const FString PluginName;

	/**
	 * Plugin content utils
	 * Point to the contents of the plugin
	*/

	/** Plugin content directory, because editor utils cannot tell us that */
	static FString PluginContentDir()
	{
		return FString::Printf(TEXT("/%s"), *PluginName);
	}

	/** Path to the plain color material asset */
	static FString PlainColorMaterialPath()
	{
		return FPaths::Combine(PluginContentDir(), PlainColorMaterialAssetName);
	}

	/** Path to the plugin specific movie pipeline config preset */
	static FString DefaultMoviePipelineConfigPath()
	{
		return FPaths::Combine(PluginContentDir(), DefaultMoviePipelineConfigAssetName);
	}

	/** Directory containing post-process materials for render targets */
	static FString PostProcessMaterialsDir()
	{
		return FPaths::Combine(PluginContentDir(), TEXT("PostProcessMaterials"));
	}

	/** Path to the specific post-process material */
	static FString PostProcessMaterialPath(const FString& TargetName)
	{
		return FPaths::Combine(PostProcessMaterialsDir(), FString::Printf(TEXT("M_PP%s"), *TargetName));
	}

	/** Clean name of the plan color material asset */
	static const FString PlainColorMaterialAssetName;

	/** Clean name of the movie pipeline config asset */
	static const FString DefaultMoviePipelineConfigAssetName;

	/**
	 * Specific project content utils
	 * Point to the content created by the plugin inside the specific project
	*/

	/**
	 * Plugin's working directory inside the specific project
	 * Can return the absolute path to the directory, or the in-game reference path
	*/
	static FString ProjectPluginContentDir(const bool bIsAbsolute = false)
	{
		const FString Prefix = (bIsAbsolute ? FPaths::ProjectContentDir() : TEXT("/Game"));
		return FPaths::Combine(Prefix, PluginName);
	}

	/** Path to the project-specific texture mapping asset */
	static FString TextureMappingAssetPath()
	{
		return FPaths::Combine(ProjectPluginContentDir(), TextureMappingAssetName);
	}

	/** Clean name of the texture mapping asset */
	static const FString TextureMappingAssetName;
};