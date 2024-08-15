#include "he_resourcemanager.hpp"

#include <set>
#include "he_io.hpp"

namespace {
	auto glslShaderDef(TextEditor& editor) {
		TextEditor::LanguageDefinition langDef;

		const char* const keywords[] = {
			"void", "bool", "int", "uint", "float", "double",
			"bvec2", "bvec3", "bvec4", "ivec2", "ivec3", "ivec4",
			"uvec2", "uvec3", "uvec4", "vec2", "vec3", "vec4",
			"dvec2", "dvec3", "dvec4", "mat2", "mat3", "mat4",
			"mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4",
			"mat4x2", "mat4x3", "mat4x4",
			"sampler2D", "in", "out", "const", "uniform", "layout", "std140",
			"if", "else", "return"
		};
		for (auto& k : keywords)
			langDef.mKeywords.insert(k);

		char const* const hyperengineMacros[] = {
			"INPUT", "OUTPUT", "VARYING", "VERT", "FRAG"
		};

		for (auto& k : hyperengineMacros) {
			TextEditor::Identifier id;
			id.mDeclaration = "HyperEngine macro";
			langDef.mPreprocIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		const char* const builtins[] = {
			"acos", "acosh", "asin", "asinh", "atan", "atanh", "cos", "cosh", "degrees", "radians", "sin", "sinh", "tan", "tanh"
			"abs", "ceil", "clamp", "dFdx", "dFdy", "exp", "exp2", "floor", "fma", "fract", "fwidth", "inversesqrt", "isinf", "isnan",
			"log", "log2", "max", "min", "mix", "mod", "modf", "noise", "pow", "round", "roundEven", "sign", "smoothstep", "sqrt",
			"step", "trunc", "floatBitsToInt", "intBitsToFloat", "gl_ClipDistance", "gl_FragCoord", "gl_FragDepth", "gl_FrontFacing",
			"gl_InstanceID", "gl_InvocationID", "gl_Layer", "gl_PointCoord", "gl_PointSize", "gl_Position", "gl_PrimitiveID",
			"gl_PrimitiveIDIn", "gl_SampleID", "gl_SampleMask", "gl_SampleMaskIn", "gl_SamplePosition", "gl_VertexID", "gl_ViewportIndex"
			"cross", "distance", "dot", "equal", "faceforward", "length", "normalize", "notEqual", "reflect", "refract", "all", "any",
			"greaterThan", "greaterThanEqual", "lessThan", "lessThanEqual", "not", "EmitVertex", "EndPrimitive", "texelFetch",
			"texelFetchOffset", "texture", "textureGrad", "textureGradOffset", "textureLod", "textureLodOffset", "textureOffset",
			"textureProj", "textureProjGrad", "textureProjGradOffset", "textureProjLod", "textureProjLodOffset", "textureProjOffset",
			"textureSize", "determinant", "inverse", "matrixCompMult", "outerProduct", "transpose",
		};
		for (auto& k : builtins) {
			TextEditor::Identifier id;
			id.mDeclaration = "Built-in";
			langDef.mIdentifiers.insert(std::make_pair(std::string(k), id));
		}

		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[ \\t]*#[ \\t]*[a-zA-Z_]+", TextEditor::PaletteIndex::Preprocessor));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("\\'\\\\?[^\\']\\'", TextEditor::PaletteIndex::CharLiteral));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", TextEditor::PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", TextEditor::PaletteIndex::Number));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", TextEditor::PaletteIndex::Identifier));
		langDef.mTokenRegexStrings.push_back(std::make_pair<std::string, TextEditor::PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", TextEditor::PaletteIndex::Punctuation));

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";

		langDef.mCaseSensitive = true;
		langDef.mAutoIndentation = true;

		langDef.mName = "GLSL";

		return langDef;
	}
}

void ResourceManager::update() {
	for (auto it = mTexturesAsserted.begin(); it != mTexturesAsserted.end();) {
		if (--it->second == 0) {
			it = mTexturesAsserted.erase(it);
		}
		else
			++it;
	}

	for (auto it = mTextures.begin(); it != mTextures.end();) {
		if (it->second.expired()) {
			it = mTextures.erase(it);
		}
		else
			++it;
	}

	for (auto it = mMeshes.begin(); it != mMeshes.end();) {
		if (it->second.expired()) {
			it = mMeshes.erase(it);
		}
		else
			++it;
	}

	for (auto it = mShaders.begin(); it != mShaders.end();) {
		if (it->second.expired()) {
			mShaderEditor.erase(it->first);
			it = mShaders.erase(it);
		}
		else
			++it;
	}
}

// Enforces the provided tetxure to stay for the next x frames
std::shared_ptr<hyperengine::Texture> ResourceManager::assertTextureLifetime(std::shared_ptr<hyperengine::Texture> const& texture, int frames) {
	if (frames > mTexturesAsserted[texture]) mTexturesAsserted[texture] = frames;
	return texture;
}

std::shared_ptr<hyperengine::Mesh> ResourceManager::getMesh(std::string_view path) {
	auto it = mMeshes.find(path);

	if (it != mMeshes.end())
		if (std::shared_ptr<hyperengine::Mesh> ptr = it->second.lock())
			return ptr;

	std::string pathStr(path);

	auto optMesh = hyperengine::readMesh(pathStr.c_str());
	if (!optMesh.has_value()) return nullptr;

	std::shared_ptr<hyperengine::Mesh> mesh = std::make_shared<hyperengine::Mesh>(std::move(optMesh.value()));
	mMeshes[std::move(pathStr)] = mesh;
	return mesh;
}

std::shared_ptr<hyperengine::Texture> ResourceManager::getTexture(std::string_view path) {
	auto it = mTextures.find(path);

	if (it != mTextures.end())
		if (std::shared_ptr<hyperengine::Texture> ptr = it->second.lock())
			return ptr;

	std::string pathStr(path);

	auto opt = hyperengine::readTextureImage(pathStr.c_str());
	if (!opt.has_value()) return nullptr;

	std::shared_ptr<hyperengine::Texture> texture = std::make_shared<hyperengine::Texture>(std::move(opt.value()));
	mTextures[std::move(pathStr)] = texture;
	return texture;
}

void ResourceManager::reloadShader(std::string const& pathStr, hyperengine::ShaderProgram& program) {
	auto shader = hyperengine::readFileString(pathStr.c_str());
	if (!shader.has_value()) return;

	// reload shader, will repopulate errors
	program = { {.source = shader.value(), .origin = pathStr } };

	// Setup debug editor
	std::unordered_map<int, std::set<std::string>> parsed;
	std::regex match("[0-9]*\\(([0-9]+)\\)");

	// On nvidia drivers, the program info log will include errors from all stages
	if (!program.errors().empty()) {
		for (auto const& error : program.errors()) {
			std::vector<std::string> lines;
			hyperengine::split(error, lines, '\n');

			// Attempt to parse information from line
			for (auto const& line : lines) {
				auto words_begin = std::sregex_iterator(line.begin(), line.end(), match);
				auto words_end = std::sregex_iterator();

				for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
					std::smatch match = *i;

					int lineMatch = std::stoi(match[1]);
					if (lineMatch == 0) lineMatch = 1; // If no line, push error to top of file
					parsed[lineMatch].insert(line);
					break; //  Only care about the first match
				}
			}
		}
	}

	TextEditor::Palette palette = { {
		0xFFB4B4B4,	// Default
		0xFFD69C56,	// Keyword	
		0xFFA8CEB5,	// Number
		0xFF859DD6,	// String
		0xFF859DD6, // Char literal
		0xFFB4B4B4, // Punctuation

		0xFF9B9B9B,	// Preprocessor

		0xffaaaaaa, // Identifier
		0xff9bc64d, // Known identifier


		0xFFFFB7BE, // Preproc identifier
		0xFF4AA657, // Comment (single line)
		0xFF4AA657, // Comment (multi line)
		0xFF1E1E1E, // Background

		0xffe0e0e0, // Cursor
		0x80a06020, // Selection
		0x800020ff, // ErrorMarker
		0x40f08000, // Breakpoint
		0xff707000, // Line number
		0x40000000, // Current line fill
		0x40808080, // Current line fill (inactive)
		0x40a0a0a0, // Current line edge
	} };

	TextEditor editor;
	editor.SetPalette(palette);

	editor.SetLanguageDefinition(glslShaderDef(editor));
	editor.SetText(shader.value());
	TextEditor::ErrorMarkers markers;
	for (auto const& [k, v] : parsed) {
		std::string str;
		for (auto const& lineFromSet : v)
			str += lineFromSet + '\n';

		markers.insert(std::make_pair(k, str));
	}
	editor.SetErrorMarkers(markers);
	bool opened = mShaderEditor[pathStr].enabled;
	mShaderEditor[pathStr] = { editor, opened || !parsed.empty() };
}

std::shared_ptr<hyperengine::ShaderProgram> ResourceManager::getShaderProgram(std::string_view path) {
	auto it = mShaders.find(path);

	if (it != mShaders.end())
		if (std::shared_ptr<hyperengine::ShaderProgram> ptr = it->second.lock())
			return ptr;

	std::string pathStr(path);

	hyperengine::ShaderProgram program;
	reloadShader(pathStr, program);

	std::shared_ptr<hyperengine::ShaderProgram> obj = std::make_shared<hyperengine::ShaderProgram>(std::move(program));
	mShaders[std::move(pathStr)] = obj;
	return obj;
}