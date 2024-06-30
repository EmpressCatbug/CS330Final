///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;

}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;
	//Create textures by loading texture jpg from folder for all necessary
	bReturn = CreateGLTexture("textures/Wood-Floor_texture2.jpg", "floor");
	bReturn = CreateGLTexture("textures/Monitor-Screen_texture.jpg", "screen");
	bReturn = CreateGLTexture("textures/Desk_texture2.jpg", "desk");
	bReturn = CreateGLTexture("textures/Keyboard_texture.jpg", "keyboard");
	bReturn = CreateGLTexture("textures/glass_texture.jpg", "glass");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

void SceneManager::LoadSceneMaterials()
{

	// Floor Material
	OBJECT_MATERIAL floorMaterial;
	floorMaterial.tag = "floorMaterial";
	floorMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	floorMaterial.ambientStrength = 0.5f;
	floorMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	floorMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	floorMaterial.shininess = 32.0f; // Increased shininess
	m_objectMaterials.push_back(floorMaterial);

	// Desk Material
	OBJECT_MATERIAL deskMaterial;
	deskMaterial.tag = "deskMaterial";
	deskMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	deskMaterial.ambientStrength = 0.5f;
	deskMaterial.diffuseColor = glm::vec3(0.6f, 0.3f, 0.3f);
	deskMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	deskMaterial.shininess = 16.0f; // Moderately shiny
	m_objectMaterials.push_back(deskMaterial);

	// Keyboard Material
	OBJECT_MATERIAL keyboardMaterial;
	keyboardMaterial.tag = "keyboardMaterial";
	keyboardMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	keyboardMaterial.ambientStrength = 0.5f;
	keyboardMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	keyboardMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	keyboardMaterial.shininess = 32.0f; 
	m_objectMaterials.push_back(keyboardMaterial);

	// Monitor Material
	OBJECT_MATERIAL monitorMaterial;
	monitorMaterial.tag = "monitorMaterial";
	monitorMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	monitorMaterial.ambientStrength = 0.5f;
	monitorMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
	monitorMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	monitorMaterial.shininess = 128.0f; // Highly shiny for a reflective look
	m_objectMaterials.push_back(monitorMaterial);

	// Screen Material (Glass-like reflective surface)
	OBJECT_MATERIAL screenMaterial;
	screenMaterial.tag = "screenMaterial";
	screenMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	screenMaterial.ambientStrength = 0.5f;
	screenMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	screenMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	screenMaterial.shininess = 256.0f; // Very shiny for strong reflections
	m_objectMaterials.push_back(screenMaterial);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	
	// load the materials for the 3D scene
	LoadSceneMaterials();
	
	// load the textures for the 3D scene
	LoadSceneTextures();

	// Setup Scene Lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{


	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Set up the floor plane
	{
		scaleXYZ = glm::vec3(50.0f, 1.0f, 50.0f);
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(0.0f, -1.0f, 0.0f);

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the floor
		SetShaderMaterial("floorMaterial");
		SetShaderTexture("floor");
		SetTextureUVScale(10.0, 10.0);

		// draw the mesh with transformation values - this plane is used for the base
		m_basicMeshes->DrawPlaneMesh();
	}

	

	// Corner piece - prism to connect the two desk surfaces
	{
		scaleXYZ = glm::vec3(12.0f, 0.5f, 7.0f); // Adjust dimensions to fill the gap
		XrotationDegrees = 0.0f;
		YrotationDegrees = 1.8f; // Rotate to align correctly
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-0.8f, 0.5f, -1.5f); // Position to fill the gap

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawPrismMesh();
	}

	

	// Keyboard Base - prism to act as base for keyboard 
	{
		scaleXYZ = glm::vec3(9.0f, 0.3f, 3.0f); // Adjust dimensions to fill the gap
		XrotationDegrees = 0.0f;
		YrotationDegrees = 1.8f; // Rotate to align correctly
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-0.8f, 1.0f, 1.5f); // Position to the corner

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// Apply the color for the rest of the keyboard (excluding the top face)
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the desk
		SetShaderMaterial("keyboardMaterial");
		m_basicMeshes->DrawBoxMesh();

		// Now, apply the texture only to the top face
		glm::vec3 topFaceScale = glm::vec3(9.0f, 0.1f, 3.0f); // Set the scale for the top face
		glm::vec3 topFacePosition = glm::vec3(-0.8f, 1.15f, 1.5f); // Slightly raise the position to cover the top face

		SetTransformations(topFaceScale, XrotationDegrees, YrotationDegrees, ZrotationDegrees, topFacePosition);
		SetShaderTexture("keyboard");
		SetTextureUVScale(1.0, 1.0);

		m_basicMeshes->DrawBoxMesh();
	}


	// Desk surface part 1 - left part of the L-shape
	{
		scaleXYZ = glm::vec3(15.0f, 0.5f, 8.8f); // Reduced dimensions
		XrotationDegrees = 0.0f;
		YrotationDegrees = 45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-8.8f, 0.5f, 4.0f); // Position adjusted to align with the other box

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Desk surface part 2 - right part of the L-shape, rotated -45 degrees
	{
		scaleXYZ = glm::vec3(15.0f, 0.5f, 8.8f); // Reduced dimensions
		XrotationDegrees = 0.0f;
		YrotationDegrees = -45.0f; // Rotate -45 degrees to form L-shape
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(7.0f, 0.5f, 4.0f); // Position adjusted to align with the other box

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Upper-Corner piece - prism to connect the two upper-desk surfaces
	{
		scaleXYZ = glm::vec3(10.0f, 0.5f, 2.5f); // Adjust dimensions to fill the gap
		XrotationDegrees = 0.0f;
		YrotationDegrees = 1.8f; // Rotate to align correctly
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-0.8f, 2.0f, -1.5f); // Position to fill the gap

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Upper Corner portion of the desk supports
	{
		glm::vec3 scaleXYZ = glm::vec3(0.5f, 1.0f, 0.4f); // Thicker and shorter dimensions
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 1.8f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(-1.0f, 1.5f, -2.0f); // Position on the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}


	// Desk upper-surface part 1 - left part of the L-shape
	{
		scaleXYZ = glm::vec3(14.0f, 0.5f, 2.5f); // Reduced dimensions
		XrotationDegrees = 0.0f;
		YrotationDegrees = 45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-9.8f, 2.0f, 3.0f); // Position adjusted to align with the other box

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Upper Left portion of the desk supports
	{
		glm::vec3 scaleXYZ = glm::vec3(0.5f, 1.0f, 0.4f); // Thicker and shorter dimensions
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 45.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(-11.4f, 1.5f, 4.0f); // Position on the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Desk upper-surface part 2 - right part of the L-shape, rotated -45 degrees
	{
		scaleXYZ = glm::vec3(14.0f, 0.5f, 2.5f); // Reduced dimensions
		XrotationDegrees = 0.0f;
		YrotationDegrees = -45.0f; // Rotate -45 degrees to form L-shape
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(8.0f, 2.0f, 3.0f); // Position adjusted to align with the other box

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Upper Right portion of the desk supports
	{
		glm::vec3 scaleXYZ = glm::vec3(0.5f, 1.0f, 0.4f); // Thicker and shorter dimensions
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = -45.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(12.4f, 1.5f, 7.5f); // Position on the desk

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.75f, 0.68f, 2.85f, 1.0f); // Lavender color for the desk
		SetShaderMaterial("deskMaterial");
		SetShaderTexture("desk");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values 
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Base 1 Corner
	{
		scaleXYZ = glm::vec3(2.0f, 0.1f, 1.0f); // Base of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = 1.8f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-0.8f, 2.4f, -1.9f); // Adjust position as necessary

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the base
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Stand 1 Corner
	{
		scaleXYZ = glm::vec3(0.2f, 2.0f, 0.2f); // Stand of the monitor
		XrotationDegrees = 0.0f; 
		YrotationDegrees = 1.8f;
		ZrotationDegrees = 0.0f; 
		positionXYZ = glm::vec3(-0.8f, 2.8f, -2.0f); // Adjust position above the base 

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); 
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the stand 
		m_basicMeshes->DrawBoxMesh(); 
	}

	// Monitor Screen 1 - Thin White Screen
	{
		glm::vec3 scaleXYZ = glm::vec3(9.0f, 2.0f, 0.2f); // Slightly smaller scale for the white screen
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 1.8f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(-1.0f, 4.5f, -1.54f); // Slightly in front of the black screen

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the SetShaderTexture("screen"); //Screensaver placeholder
		SetShaderMaterial("screenMaterial");
		SetShaderTexture("screen"); //Screensaver placeholder
		//SetShaderTexture("glass");
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values - this plane is used for the base
		m_basicMeshes->DrawBoxMesh();
	}


	// Monitor Screen 1 Corner
	{
		scaleXYZ = glm::vec3(10.0f, 3.0f, 0.4f); // Screen of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = 1.8f; 
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-1.0f, 4.5f, -1.7f); // Position above the stand

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the screen
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Base 2 Left
	{
		scaleXYZ = glm::vec3(2.0f, 0.1f, 1.0f); // Base of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = 45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-11.0f, 2.4f, 2.92f); // Adjust position as necessary

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the base
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Stand 2 Left
	{
		scaleXYZ = glm::vec3(0.2f, 2.0f, 0.2f); // Stand of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = 45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-11.0f, 2.8f, 2.6f); // Adjust position above the base 

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the stand 
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Screen 2 - Thin White Screen
	{
		glm::vec3 scaleXYZ = glm::vec3(8.8f, 2.0f, 0.2f); // Slightly smaller scale for the white screen
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = 45.0f;
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(-10.25f, 4.5f, 3.5f); // Slightly in front of the black screen

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the screen
		SetShaderMaterial("screenMaterial");
		SetShaderTexture("screen"); //Screensaver placeholder
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values - this plane is used for the base
		m_basicMeshes->DrawBoxMesh();
	}


	// Monitor Screen 2 Left
	{
		scaleXYZ = glm::vec3(10.0f, 3.0f, 0.4f); // Screen of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = 45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(-10.8f, 4.5f, 3.0f); // Position above the stand

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the screen
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Base 3 Right
	{
		scaleXYZ = glm::vec3(2.0f, 0.1f, 1.0f); // Base of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = -45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(8.8f, 2.4f, 2.6f); // Adjust position as necessary

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the base
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Stand 3 Right
	{
		scaleXYZ = glm::vec3(0.2f, 2.0f, 0.2f); // Stand of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = -45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(8.8f, 2.8f, 2.4f); // Adjust position above the base 

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the stand 
		m_basicMeshes->DrawBoxMesh();
	}

	// Monitor Screen 3 - Thin White Screen
	{
		glm::vec3 scaleXYZ = glm::vec3(8.8f, 2.0f, 0.2f); // Slightly smaller scale for the white screen
		float XrotationDegrees = 0.0f;
		float YrotationDegrees = -45.0f; // Opposite rotation to fit on the other side
		float ZrotationDegrees = 0.0f;
		glm::vec3 positionXYZ = glm::vec3(8.4f, 4.55f, 3.5f); // Adjust position to fit on the right monitor

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // White color for the screen
		SetShaderMaterial("screenMaterial");
		SetShaderTexture("screen"); //Screensaver placeholder
		SetTextureUVScale(1.0, 1.0);

		// draw the mesh with transformation values - this plane is used for the base
		m_basicMeshes->DrawBoxMesh();
	}


	// Monitor Screen 3 Right
	{
		scaleXYZ = glm::vec3(10.0f, 3.0f, 0.4f); // Screen of the monitor
		XrotationDegrees = 0.0f;
		YrotationDegrees = -45.0f;
		ZrotationDegrees = 0.0f;
		positionXYZ = glm::vec3(8.8f, 4.5f, 3.0f); // Position above the stand

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f); // Silver color for the screen
		m_basicMeshes->DrawBoxMesh();
	}

	
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is used for setting up the light sources in
 *  the 3D scene.
 ***********************************************************/

void SceneManager::SetupSceneLights()
{
	if (m_pShaderManager == NULL) return;

	// Enable lighting
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Global ambient light
	glm::vec3 globalAmbient(0.2f, 0.2f, 0.2f); // Moderate ambient light
	m_pShaderManager->setVec3Value("globalAmbientColor", globalAmbient);

	// Light 0 - Key Light (main soft white light from above)
	glm::vec3 keyLightPos(0.0f, 12.0f, 0.0f);
	glm::vec3 keyLightDiffuse(0.4f, 0.4f, 0.4f); // Increased brightness
	glm::vec3 keyLightSpecular(7.0f, 7.0f, 7.0f);
	m_pShaderManager->setVec3Value("lightSources[0].position", keyLightPos);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", keyLightDiffuse);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", keyLightSpecular);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.2f); // Increased specular intensity

	// Light 1 - Warm Light under the upper part of the desk (soft yellow glow)
	glm::vec3 warmLightPos(-9.8f, 2.0f, 3.0f); // Adjusted position
	glm::vec3 warmLightDiffuse(1.0f, 0.85f, 0.5f); // Soft yellow color
	glm::vec3 warmLightSpecular(1.0f, 0.85f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[1].position", warmLightPos);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", warmLightDiffuse);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", warmLightSpecular);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f); // Softer specular intensity

	// Light 2 - Monitor Light (Left monitor)
	glm::vec3 leftMonitorPos(8.0f, 2.0f, 3.0f); // Adjusted position
	glm::vec3 leftMonitorDiffuse(0.6f, 0.8f, 1.0f); // Cool light color
	glm::vec3 leftMonitorSpecular(0.6f, 0.8f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[2].position", leftMonitorPos);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", leftMonitorDiffuse);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", leftMonitorSpecular);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f); // Increased intensity

	// Light 3 - Monitor Light (Center monitor)
	//glm::vec3 centerMonitorPos(-0.8f, 2.0f, -1.5f); // Adjusted position
	//glm::vec3 centerMonitorDiffuse(0.6f, 0.8f, 1.0f); // Cool light color
	//glm::vec3 centerMonitorSpecular(0.6f, 0.8f, 1.0f);
	//m_pShaderManager->setVec3Value("lightSources[3].position", centerMonitorPos);
	//m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", centerMonitorDiffuse);
	//m_pShaderManager->setVec3Value("lightSources[3].specularColor", centerMonitorSpecular);
	//m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 32.0f);
	//m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.2f); // Increased intensity

	// Light 4 - Monitor Light (Right monitor)
	//glm::vec3 rightMonitorPos(1.5f, 1.2f, 0.0f); // Adjusted position
	//glm::vec3 rightMonitorDiffuse(0.6f, 0.8f, 1.0f); // Cool light color
	//glm::vec3 rightMonitorSpecular(0.6f, 0.8f, 1.0f);
	//m_pShaderManager->setVec3Value("lightSources[4].position", rightMonitorPos);
	//m_pShaderManager->setVec3Value("lightSources[4].diffuseColor", rightMonitorDiffuse);
	//m_pShaderManager->setVec3Value("lightSources[4].specularColor", rightMonitorSpecular);
	//m_pShaderManager->setFloatValue("lightSources[4].focalStrength", 32.0f);
	//m_pShaderManager->setFloatValue("lightSources[4].specularIntensity", 0.2f); // Increased intensity
}