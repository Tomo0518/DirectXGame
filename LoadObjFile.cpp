#include "LoadObjFile.h"
#include "LoadMaterialTemplateFile.h"

#include <fstream>
#include <sstream>
#include <cassert>

ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName) {
	// ================================
	// 1.必要な変数の宣言
	// ================================
	ModelData modelData; // 構築するModelData
	std::vector<Vector4> positions; // 頂点座標格
	std::vector<Vector3> normals;   // 法線ベクトル格納
	std::vector<Vector2> texcoords; // UV座標格納
	std::string line; // ファイルから読み込んだ1行分の文字列


	// ================================
	// 2.ファイルを開く
	// ================================
	std::ifstream file(directoryPath + "/" + fileName); // ファイルを開く
	assert(file.is_open()); // ファイルが開けなかった場合は停止

	// ================================
	// 3.ファイルを読みモデルデータを構築
	// ================================
	while (std::getline(file, line)) {
		std::string identifier; // 行の先頭識別子
		std::istringstream s(line);
		s >> identifier; // 行の先頭識別子を取得

		// identifierごとに応じた処理

		if (identifier == "v") {
			// 頂点座標
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f; // w成分は1.0fに設定
			position.x *= -1.0f;
			positions.push_back(position);

		}
		else if (identifier == "vt") { // UV座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") { // 法線ベクトル
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") { // 面情報
			std::vector<VertexData> polygon;

			// 面の全頂点を読み込む（三角形、四角形、多角形に対応）
			std::string vertexDefinition;
			while (s >> vertexDefinition) {
				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}

				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				polygon.push_back({ position, texcoord, normal });
			}

			// 多角形を三角形に分割（Fan Triangulation）
			// 頂点が v0, v1, v2, v3 の四角形の場合
			// 三角形1: v0, v1, v2
			// 三角形2: v0, v2, v3
			for (size_t i = 1; i + 1 < polygon.size(); ++i) {
				modelData.vertices.push_back(polygon[0]);      // 基準頂点（逆順のため最後）
				modelData.vertices.push_back(polygon[i + 1]);  // 次の頂点
				modelData.vertices.push_back(polygon[i]);      // 現在の頂点
			}
		}
		else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるのでディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;
}