#include <iostream>
#include <sstream>
#include <string>

#define _EASYBLENDSDK_LINUX
#include "EasyBlendSDK.h"

EasyBlendSDK_Mesh * initScalableMesh(std::string displayName, std::string specifier)
{
	//std::cout << displayName << std::endl;	

	std::string projectors[] = {"cave010:0.0", "cave010:0.1", "cave010:0.2", "cave009:0.0", "cave009:0.1", "cave009:0.2", "cave009:0.3", "cave008:0.0", "cave008:0.1", "cave008:0.2", "cave007:0.0", "cave007:0.1", "cave007:0.3", "cave007:0.2", "cave006:0.0", "cave006:0.1", "cave006:0.3", "cave006:0.2", "cave005:0.0", "cave005:0.1", "cave005:0.3", "cave005:0.2", "cave004:0.0", "cave004:0.1", "cave004:0.3", "cave004:0.2", "cave003:0.0", "cave003:0.1", "cave003:0.3", "cave003:0.2", "cave002:0.0", "cave002:0.1", "cave002:0.3", "cave002:0.2", "cave001:0.0", "cave001:0.1", "cave001:0.3", "cave001:0.2"};

	std::string POLfileName;

	for(int i = 0; i < 38; i++)
	{
		if(projectors[i] == displayName)
		{
			std::stringstream sstm;
			sstm.str(std::string());
			sstm << "/gpfs/home/cavedemo/scalable/cave/ScalableData.pol_" << i;
			POLfileName = sstm.str(); 
		}
	}

	//std::cout << "POL Filename: " << POLfileName << std::endl;

	EasyBlendSDK_Mesh *gMSDK = new EasyBlendSDK_Mesh;
	EasyBlendSDKError msdkErr;

	msdkErr = EasyBlendSDK_Initialize(POLfileName.c_str(), gMSDK);
	if(msdkErr != EasyBlendSDK_ERR_S_OK)
	{
		std::cout << "Error on EasyBlendSDK_Initialize: " << EasyBlendSDK_GetErrorMessage(msdkErr) << std::endl;
		std::cout << "File is: " << POLfileName.c_str() << std::endl;
	}

	//std::cout << "Mesh Initialized!" << std::endl;

	if(specifier == "left")
	{
		EasyBlendSDK_SetInputReadBuffer(gMSDK, GL_BACK_LEFT);
		EasyBlendSDK_SetOutputDrawBuffer(gMSDK,  GL_BACK_LEFT);
	}
	else if(specifier == "right")
	{
		EasyBlendSDK_SetInputReadBuffer(gMSDK, GL_BACK_RIGHT);
		EasyBlendSDK_SetOutputDrawBuffer(gMSDK,  GL_BACK_RIGHT);
	}
	else
	{
		EasyBlendSDK_SetInputReadBuffer(gMSDK, GL_BACK);
		EasyBlendSDK_SetOutputDrawBuffer(gMSDK,  GL_BACK);
	}

	//std::cout << "Buffers Set!" << std::endl;

	return gMSDK;
}
