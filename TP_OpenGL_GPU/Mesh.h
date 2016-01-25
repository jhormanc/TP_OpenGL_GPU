#pragma once

struct Mesh
{
	std::vector<glm::vec4> vertices;
	std::vector<glm::vec2> textures;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec4> vertices_indexed;
	std::vector<glm::vec2> textures_indexed;
	std::vector<glm::vec3> normals_indexed;
	std::vector<GLuint> verticesIndex;
	std::vector<GLuint> texturesIndex;
	std::vector<GLuint> normalsIndex;
	std::vector<GLuint> texturesNumber;
	std::vector<GLuint> texturesNumber_indexed;

	Mesh() :
		vertices(std::vector<glm::vec4>()),
		textures(std::vector<glm::vec2>()),
		normals(std::vector<glm::vec3>()),
		verticesIndex(std::vector<GLuint>()),
		texturesIndex(std::vector<GLuint>()),
		normalsIndex(std::vector<GLuint>()),
		texturesNumber(std::vector<GLuint>())
	{

	}

	Mesh(std::vector<glm::vec4> _vertices, std::vector<glm::vec2> _textures, std::vector<glm::vec3> _normals,
		std::vector<GLuint> _verticesIndex, std::vector<GLuint> _texturesIndex, std::vector<GLuint> _normalsIndex, std::vector<GLuint> _texturesNumber) :
		vertices(_vertices),
		textures(_textures),
		normals(_normals),
		verticesIndex(_verticesIndex),
		texturesIndex(_texturesIndex),
		normalsIndex(_normalsIndex),
		texturesNumber(_texturesNumber)
	{

	}

	void merge(Mesh* m)
	{
		int vertices_size = static_cast<int>(vertices.size());
		int textures_size = static_cast<int>(textures.size());
		int normals_size = static_cast<int>(normals.size());

		for (int i = 0; i < m->vertices.size(); i++)
			vertices.push_back(m->vertices[i]);

		for (int i = 0; i < m->textures.size(); i++)
			textures.push_back(m->textures[i]);

		for (int i = 0; i < m->normals.size(); i++)
			normals.push_back(m->normals[i]);

		for (int i = 0; i < m->verticesIndex.size(); i++)
			verticesIndex.push_back(vertices_size + m->verticesIndex[i]);

		for (int i = 0; i < m->texturesIndex.size(); i++)
			texturesIndex.push_back(textures_size + m->texturesIndex[i]);

		for (int i = 0; i < m->normalsIndex.size(); i++)
			normalsIndex.push_back(normals_size + m->normalsIndex[i]);

		for (int i = 0; i < m->texturesNumber.size(); i++)
			texturesNumber.push_back(m->texturesNumber[i]);
	}

	static Mesh* Quadrangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const int num_texture)
	{
		std::vector<glm::vec4> points(4);
		points[0] = glm::vec4(p0.x, p0.y, p0.z, 1.f);
		points[1] = glm::vec4(p1.x, p1.y, p1.z, 1.f);
		points[2] = glm::vec4(p2.x, p2.y, p2.z, 1.f);
		points[3] = glm::vec4(p3.x, p3.y, p3.z, 1.f);

		std::vector<GLuint> faces(6);
		faces[0] = 1;
		faces[1] = 2;
		faces[2] = 3;
		faces[3] = 2;
		faces[4] = 4;
		faces[5] = 3;

		glm::vec3 normalT1 = glm::normalize(glm::cross(p1 - p0, p3 - p0));
		glm::vec3 normalT2 = glm::normalize(glm::cross(p1 - p2, p3 - p2));
		glm::vec3 normalMiddle = glm::normalize((normalT1 + normalT2) * 0.5f);
		std::vector<glm::vec3> normals(4);
		normals[0] = normalT1;
		normals[1] = normalMiddle;
		normals[2] = normalMiddle;
		normals[3] = normalT2;

		std::vector<GLuint> facesNormals(6);
		facesNormals[0] = 1;
		facesNormals[1] = 2;
		facesNormals[2] = 3;
		facesNormals[3] = 2;
		facesNormals[4] = 4;
		facesNormals[5] = 3;

		std::vector<glm::vec2> UVTextures(4);
		UVTextures[0] = glm::vec2(0, 1);
		UVTextures[1] = glm::vec2(1, 1);
		UVTextures[2] = glm::vec2(1, 0);
		UVTextures[3] = glm::vec2(0, 0);

		std::vector<GLuint> texturesIndex(6);
		texturesIndex[0] = 1;
		texturesIndex[1] = 2;
		texturesIndex[2] = 3;
		texturesIndex[3] = 2;
		texturesIndex[4] = 4;
		texturesIndex[5] = 3;

		std::vector<GLuint> number(6);
		number[0] = num_texture;
		number[1] = num_texture;
		number[2] = num_texture;
		number[3] = num_texture;
		number[4] = num_texture;
		number[5] = num_texture;

		return new Mesh(points, UVTextures, normals, faces, texturesIndex, facesNormals, number);
	}

	void indexData()
	{
		vertices_indexed = std::vector<glm::vec4>();
		textures_indexed = std::vector<glm::vec2>();
		normals_indexed = std::vector<glm::vec3>();
		texturesNumber_indexed = std::vector<GLuint>();

		if (vertices.size() > 0)
		{
			if (verticesIndex.size() > 0)
			{
				for (unsigned int i = 0; i < verticesIndex.size(); i++)
				{
					unsigned int vertexIndex = verticesIndex[i];
					glm::vec4 vertex = vertices[vertexIndex - 1];
					vertices_indexed.push_back(vertex);
					GLuint num = texturesNumber[vertexIndex - 1];
					texturesNumber_indexed.push_back(num);
				}
			}
			else
			{
				for (unsigned int i = 0; i < vertices.size(); i++)
				{
					glm::vec4 vertex = vertices[i];
					vertices_indexed.push_back(vertex);
					GLint num = texturesNumber[i];
					texturesNumber_indexed.push_back(num);
				}
			}
		}

		if (textures.size() > 0)
		{
			if (texturesIndex.size() > 0)
			{
				for (unsigned int i = 0; i < texturesIndex.size(); i++)
				{
					unsigned int uvIndex = texturesIndex[i];
					glm::vec2 uv = textures[uvIndex - 1];
					textures_indexed.push_back(uv);
				}
			}
			else
			{
				for (unsigned int i = 0; i < textures.size(); i++)
				{
					glm::vec2 uv = textures[i];
					textures_indexed.push_back(uv);
				}
			}
		}

		if (normals.size() > 0)
		{
			if (normalsIndex.size() > 0)
			{
				for (unsigned int i = 0; i < normalsIndex.size(); i++)
				{
					unsigned int normalIndex = normalsIndex[i];
					glm::vec3 normal = normals[normalIndex - 1];
					normals_indexed.push_back(normal);
				}
			}
			else
			{
				for (unsigned int i = 0; i < normals.size(); i++)
				{
					glm::vec3 normal = normals[i];
					normals_indexed.push_back(normal);
				}
			}
		}
	}

	void SetMeshModel(const glm::vec3 &pos = glm::vec3(0.f), const glm::vec3 &scale = glm::vec3(1.f), const float rot_angle = 0.f, const glm::vec3 &rot_axes = glm::vec3(0.f))
	{

	}
};