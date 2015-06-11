#ifdef USE_SCALABLE
#ifdef WIN32
#  include <windows.h>
#endif

#ifdef WIN32
#include <GL/gl.h>
#include "glext.h"
#else
#include <GL/gl.h>
#include <GL/glx.h>
#endif


#include <string>
#include <set>

#ifdef USE_SCALABLE

#include "EasyBlendSDK.h"
#include "Scalable.h" 
#include <stdio.h>
#include <math.h>
#include <iostream>

EasyBlendSDK_Mesh *gMSDK_left;
EasyBlendSDK_Mesh *gMSDK_right;
EasyBlendSDK_Frustum Frustum_left;
EasyBlendSDK_Frustum Frustum_right;
EasyBlendSDKError msdkErr;

#endif
#include <stdio.h>
#include <math.h>
#include <iostream>
using namespace std;

// global variables used for the EasyBlend SDK toolkit.
bool useScalable;
double LookVec[3];

/* ====================================================================== */
void ScalableInit(const char* ScalableMesh) {
#ifdef USE_SCALABLE
  gMSDK_left = new EasyBlendSDK_Mesh;
  gMSDK_right = new EasyBlendSDK_Mesh;
 
  useScalable = true;
  LookVec[0] = 0;  LookVec[1] = 0;  LookVec[2] = -8; 
  //std::cout << "Mesh file = " << ScalableMesh << std::endl;
  
  msdkErr = EasyBlendSDK_Initialize(ScalableMesh, gMSDK_left );
  if ( msdkErr != EasyBlendSDK_ERR_S_OK )
    {
      std::cout << "Error on left Init: " << EasyBlendSDK_GetErrorMessage(msdkErr)
                << std::endl;
      std::cout << "File is: " << ScalableMesh <<std::endl;
	  useScalable = false;
	  return;
  }
  
  msdkErr = EasyBlendSDK_Initialize(ScalableMesh, gMSDK_right );
  if ( msdkErr != EasyBlendSDK_ERR_S_OK )
    {
      std::cout << "Error on right Init: " << EasyBlendSDK_GetErrorMessage(msdkErr)
                << std::endl;
      std::cout << "File is: " << ScalableMesh <<std::endl;
	  useScalable = false;
	  return;
  }
  // Expecting a projective mesh
  if (gMSDK_left->Projection != EasyBlendSDK_PROJECTION_Perspective)
  {
    std::cout << "Expected projective mesh for left eye" << std::endl;
    useScalable = false;
    return;
  }
  
  if (gMSDK_right->Projection != EasyBlendSDK_PROJECTION_Perspective)
  {
    std::cout << "Expected projective mesh for right eye" << std::endl;
    useScalable = false;
    return;
  }
  
   EasyBlendSDK_SetInputReadBuffer(gMSDK_left,  GL_BACK_LEFT);
   EasyBlendSDK_SetOutputDrawBuffer(gMSDK_left,  GL_BACK_LEFT);

   EasyBlendSDK_SetInputReadBuffer(gMSDK_right,  GL_BACK_RIGHT);
   EasyBlendSDK_SetOutputDrawBuffer(gMSDK_right, GL_BACK_RIGHT);
    
#endif
}

/* ====================================================================== */
void ScalablePreSwap(bool left) {
	#ifdef USE_SCALABLE
  // Set which buffer to read from, and which buffer to write back into
  //  for warping and blending.  This is done before the buffer is
  //  swapped, and once for each eye.  
  if (useScalable) {
    glFlush();
    if(left){
    EasyBlendSDK_TransformInputToOutput(gMSDK_left);
  }else{
    EasyBlendSDK_TransformInputToOutput(gMSDK_right);
  }
  glFlush();
  }
#endif
}

void ScalableSetEye(bool left) {
      if (left)
      Frustum_left= gMSDK_left->Frustum;
    else
      Frustum_right= gMSDK_right->Frustum;
} 

void ScalableSetView0(double gEyeX, double gEyeY, double gEyeZ, bool left) 
{
#ifdef USE_SCALABLE
  if (!useScalable)
    return;
    
  EasyBlendSDKError err;
 
  if (left)
    err = EasyBlendSDK_SetEyepoint(gMSDK_left,gEyeX,gEyeY,gEyeZ);
  else
    err = EasyBlendSDK_SetEyepoint(gMSDK_right,gEyeX,gEyeY,gEyeZ);
 
  if ( err != EasyBlendSDK_ERR_S_OK ) {
    std::cout << "Error on Setting EyePoint: " 
      << EasyBlendSDK_GetErrorMessage(err)
      << std::endl;
  }
#endif
}


void rotateVec(double angle, double axisX, double axisY, double axisZ, double &vecX, double &vecY, double &vecZ)
{
	double c = cos(angle);
	double s = sin(angle);
	double t = 1.0 - c;

	//  if axis is not already normalised then uncomment this
	// double magnitude = Math.sqrt(axisX*axisX + axisY*axisY + axisZ*axisZ);
	// if (magnitude==0) throw error;
	// axisX /= magnitude;
	// axisY /= magnitude;
	// axisZ /= magnitude;

	double m00 = c + axisX*axisX*t;
	double m11 = c + axisY*axisY*t;
	double m22 = c + axisZ*axisZ*t;

	double tmp1 = axisX*axisY*t;
	double tmp2 = axisZ*s;
	double m10 = tmp1 + tmp2;
	double m01 = tmp1 - tmp2;
	tmp1 = axisX*axisZ*t;
	tmp2 = axisY*s;
	double m20 = tmp1 - tmp2;
	double m02 = tmp1 + tmp2;    tmp1 = axisY*axisZ*t;
	tmp2 = axisX*s;
	double m21 = tmp1 + tmp2;
	double m12 = tmp1 - tmp2;

	tmp1 = vecX;
	tmp2 = vecY;
	double tmp3 = vecZ;

	vecX = m00 * tmp1 + m01 * tmp2 + m02 * tmp3;
	vecY = m10 * tmp1 + m11 * tmp2 + m12 * tmp3;
	vecZ = m20 * tmp1 + m21 * tmp2 + m22 * tmp3;
}

void computeTileCornerPoint(double xang, double yang, double &x, double &y, double &z, bool left)
{
    
    const double deg2rad = M_PI/180.0;

	x = -tan(deg2rad * xang);
	y = -tan(deg2rad * yang);
	z = 1.0f;
	if(left){ 
		rotateVec(-deg2rad*(Frustum_left.ViewAngleA), 0, 0, 1, x, y, z);
		rotateVec(-deg2rad*(Frustum_left.ViewAngleB), 0, 1, 0, x, y, z);
		rotateVec(-deg2rad*(Frustum_left.ViewAngleC + 90), 1, 0, 0, x, y, z);
	}else{
		rotateVec(-deg2rad*(Frustum_right.ViewAngleA), 0, 0, 1, x, y, z);
		rotateVec(-deg2rad*(Frustum_right.ViewAngleB), 0, 1, 0, x, y, z);
		rotateVec(-deg2rad*(Frustum_right.ViewAngleC + 90), 1, 0, 0, x, y, z);
	}
}

void getTopLeft(double &x, double &y, double &z, bool left) {
	if(left){ 
		computeTileCornerPoint(Frustum_left.LeftAngle, Frustum_left.TopAngle, x, y, z, left);
	}else{
		computeTileCornerPoint(Frustum_right.LeftAngle, Frustum_right.TopAngle, x, y, z, left);
	}
}

void getTopRight(double &x, double &y, double &z, bool left) {
	if(left){ 
		computeTileCornerPoint(Frustum_left.RightAngle, Frustum_left.TopAngle, x, y, z, left);
	}else{
		computeTileCornerPoint(Frustum_right.RightAngle, Frustum_right.TopAngle, x, y, z, left);
	}
}

void getBotLeft(double &x, double &y, double &z, bool left) {
	if(left){ 
		computeTileCornerPoint(Frustum_left.LeftAngle, Frustum_left.BottomAngle, x, y, z, left);
	}else{
		computeTileCornerPoint(Frustum_right.LeftAngle, Frustum_right.BottomAngle, x, y, z, left);
	}
}

void getBotRight(double &x, double &y, double &z, bool left) {
	if(left){ 
		computeTileCornerPoint(Frustum_left.RightAngle, Frustum_left.BottomAngle, x, y, z, left);
	}else{
		computeTileCornerPoint(Frustum_right.RightAngle, Frustum_right.BottomAngle, x, y, z, left);
	}
}


/* ====================================================================== */
void ScalableClose() {
#ifdef USE_SCALABLE
	  // uninitialize the EasyBlend SDK toolkit and free up the memory
  msdkErr = EasyBlendSDK_Uninitialize( gMSDK_left );
  if ( msdkErr != EasyBlendSDK_ERR_S_OK )
  {
    std::cout << "There was error uninitializing " << msdkErr 
      << " " << EasyBlendSDK_GetErrorMessage(msdkErr) << std::endl;
  }  
  msdkErr = EasyBlendSDK_Uninitialize( gMSDK_right );
  if ( msdkErr != EasyBlendSDK_ERR_S_OK )
  {
    std::cout << "There was error uninitializing " << msdkErr 
      << " " << EasyBlendSDK_GetErrorMessage(msdkErr) << std::endl;
  }    
  delete gMSDK_left;
  delete gMSDK_right;
  //CleanupFrameBufferObjects();
#endif
}

#endif
