
#include "shader.h"

//TODO: Make a map
//list of created shaders
std::map<std::string,Shader> Shader::createdShaders;

// constructor generates the shader on the fly
// ------------------------------------------------------------------------
Shader::Shader(std::string shader_name, const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{

	uniform_floats["iTime"] = 0.0f;
	uniform_floats["iFrame"] = 0.0f;
	// 1. retrieve the vertex/fragment source code from filePath
	std::string vertexCode;
	std::string fragmentCode;
	std::string geometryCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;
	std::ifstream gShaderFile;
	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// open files
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
		// if geometry shader path is present, also load a geometry shader
		if (geometryPath != nullptr)
		{
			gShaderFile.open(geometryPath);
			std::stringstream gShaderStream;
			gShaderStream << gShaderFile.rdbuf();
			gShaderFile.close();
			geometryCode = gShaderStream.str();
		}
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();
	// 2. compile shaders
	unsigned int vertex, fragment;
	// vertex shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");
	// fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");
	// if geometry shader is given, compile geometry shader
	unsigned int geometry;
	if (geometryPath != nullptr)
	{
		const char * gShaderCode = geometryCode.c_str();
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);
		checkCompileErrors(geometry, "GEOMETRY");
	}
	// shader Program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	if (geometryPath != nullptr)
		glAttachShader(ID, geometry);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessery
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	if (geometryPath != nullptr)
		glDeleteShader(geometry);


	Shader::createdShaders.insert({ shader_name, *this });

}

Shader::Shader(std::string shader_name, const char* computePath) {


	uniform_floats["iTime"] = 0.0f;
	uniform_floats["iFrame"] = 0.0f;

	// 1. retrieve the compute source code from filePath
	std::string computeCode;

	std::ifstream cShaderFile;

	// ensure ifstream objects can throw exceptions:
	cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// open files
		cShaderFile.open(computePath);
		std::stringstream cShaderStream;

		// read file's buffer contents into streams
		cShaderStream << cShaderFile.rdbuf();

		// close file handlers
		cShaderFile.close();

		// convert stream into string
		computeCode = cShaderStream.str();

	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	const char* cShaderCode = computeCode.c_str();

	// 2. compile shaders
	unsigned int compute;
	// vertex shader
	compute = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(compute, 1, &cShaderCode, NULL);
	glCompileShader(compute);
	checkCompileErrors(compute, "COMPUTE");

	// shader Program
	ID = glCreateProgram();
	glAttachShader(ID, compute);

	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessery
	glDeleteShader(compute);

	Shader::createdShaders.insert({shader_name, *this });
}

// activate the shader
// ------------------------------------------------------------------------
void Shader::use()
{
	glUseProgram(ID);
}
// utility uniform functions
// ------------------------------------------------------------------------
void Shader::setBool(const std::string name, bool value)
{
	uniform_bool[name] = value;
}
// ------------------------------------------------------------------------
void Shader::setInt(const std::string name, int value)
{
	uniform_ints[name] = value;
}
// ------------------------------------------------------------------------
void Shader::setFloat(const std::string name, float value)
{
	uniform_floats[name] = value;
}
// ------------------------------------------------------------------------
void Shader::setVec2(const std::string name, const glm::vec2 value)
{
	uniform_vec2[name] = value;
}
void Shader::setVec2(const std::string name, float x, float y)
{
	uniform_vec2[name] = glm::vec2(x, y);
}
// ------------------------------------------------------------------------
void Shader::setVec3(const std::string name, const glm::vec3 value)
{
	uniform_vec3[name] = value;
}
void Shader::setVec3(const std::string name, float x, float y, float z)
{
	uniform_vec3[name] = glm::vec3(x, y, z);
}
// ------------------------------------------------------------------------
void Shader::setVec4(const std::string name, const glm::vec4 value)
{
	uniform_vec4[name] = value;
}
void Shader::setVec4(const std::string name, float x, float y, float z, float w)
{
	uniform_vec4[name] = glm::vec4(x, y, z, w);
}
// ------------------------------------------------------------------------
void Shader::setMat2(const std::string name, const glm::mat2 mat)
{
	glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void Shader::setMat3(const std::string name, const glm::mat3 mat)
{
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void Shader::setMat4(const std::string name, const glm::mat4 mat)
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::updateUniforms() {
	++uniform_floats["iTime"];
	++uniform_floats["iFrame"];
	for (const auto &element : uniform_bool) {
		glUniform1i(glGetUniformLocation(ID, element.first.c_str()), (int)element.second);
	}
	
	for (const auto &element : uniform_ints) {
		glUniform1i(glGetUniformLocation(ID, element.first.c_str()), element.second);
	}

	for (const auto &elements : uniform_floats) {
		glUniform1f(glGetUniformLocation(ID, elements.first.c_str()), elements.second);
	}

	for (const auto &elements : uniform_vec2) {
		glUniform2fv(glGetUniformLocation(ID, elements.first.c_str()), 1, &elements.second[0]);
	}

	for (const auto &elements : uniform_vec3) {
		glUniform3fv(glGetUniformLocation(ID, elements.first.c_str()), 1, &elements.second[0]);
	}

	for (const auto &elements : uniform_vec4) {
		glUniform4fv(glGetUniformLocation(ID, elements.first.c_str()), 1, &elements.second[0]);
	}
}

// utility function for checking shader compilation/linking errors.
// ------------------------------------------------------------------------
void Shader::checkCompileErrors(GLuint shader, std::string type)
{
	GLint success;
	GLchar infoLog[1024];
	if (type != "PROGRAM")
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
}


