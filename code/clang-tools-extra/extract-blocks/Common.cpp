#include "Common.h"


json Variable::toJson() {
    json posArray = json::array();
    for (auto& pos : locations) {
        posArray.push_back(pos.getIndex());
    }

    json j;
    j["name"] = name;
    j["type"] = type;
    j["locations"] = posArray;
    return j;
}


json Block::toJson() {
    json preVarArray = json::array();
    json postVarArray = json::array();

    for (auto& var : preContext) {
        preVarArray.push_back(var.toJson());
    }
    for (auto& var : postContext) {
        postVarArray.push_back(var.toJson());
    }

    json j;
    j["code"] = code;
    j["preContext"] = preVarArray;
    j["postContext"] = postVarArray;

    return j;
}


string readFileToString(const string& filePath) {
    ifstream inFile(filePath);
    if (!inFile.is_open()) {
        cerr << "Failed to open the file: " << filePath << endl;
        //return "";
        exit(1);
    }

    stringstream buffer;
    buffer << inFile.rdbuf();
    
    return buffer.str();
}


bool isVarExist(const string& varName, const vector<Variable>& context) {
    return find_if(context.begin(), context.end(),
                        [&varName](const Variable& varObj) {
                            return varObj.getName() == varName;
                        }) != context.end();
}


vector<Pos> findVarPositions(const string& code, const string& var) {
    std::vector<Pos> positions;
    size_t index = code.find(var, 0);

    while (index != string::npos) {
        bool isStart = (index == 0 || (!isalnum(code[index - 1]) && code[index-1] != '_'));
        bool isEnd = (index + var.size() == code.size() || (!isalnum(code[index + var.size()]) && code[index + var.size()] != '_'));
        // bool isStart = (index == 0 || !isalnum(code[index - 1]) || code[index-1] == '_');
        // bool isEnd = (index + var.size() == code.size() || !isalnum(code[index + var.size()]) || code[index + var.size()] == '_');

        if (isStart && isEnd) {
            Pos pos(index);
            positions.push_back(pos);
        }

        index = code.find(var, index + 1);
    }

    return positions;
}


string replaceSubstring(const std::string& source,
                             const std::string& from,
                             const std::string& to) {
    std::string result = source;
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();  // Move the position past the replacement
    }

    return result;
}


void writeJsonFile(const string& sourcePath, const string& outputRootPath, vector<Block>& blockList) {
    string::size_type iPos = sourcePath.find_last_of('/') + 1;
    string sourceFilename = sourcePath.substr(iPos, sourcePath.length() - iPos);
    string sourcePrefix = sourceFilename.substr(0, sourceFilename.rfind("."));
    //string outputDirPath = outputRootPath + "/" + outputDirName;

    /*
    if (!filesystem::exists(outputDirPath)) {
        if (!filesystem::create_directory(outputDirPath)) {
            cerr << "Failed to create the directory '" << outputDirPath << "'." << endl;
            //return;
            exit(1);
        }
    } 
    */

    int blockID = 0;
    for (auto& block : blockList) {
        json jBlock = block.toJson();
        string outputFileName = sourcePrefix + "-" + to_string(blockID) + ".json";
        string outputFilePath = outputRootPath + "/" + outputFileName;

        ofstream file(outputFilePath);
        if (file.is_open()) {
            file << jBlock.dump(2);  // 2 spaces for indentation
            file.close();
            cout << "Write to " << outputFilePath << endl;
        } else {
            cerr << "Failed to open the file." << endl;
            exit(1);
        }
        blockID++;
    }

    // json jBlocks = json::array();
    // for (auto& block : blockList) {
    //     jBlocks.push_back(block.toJson());
    // }

    // json jBlockList;
    // jBlockList["blocks"] = jBlocks;

    // cout << jBlockList.dump(2) << endl;

    // write to json file
    // size_t pos = path.rfind('.');
    // string jsonFilename = path.substr(0, pos) + ".json";

    /*
    ofstream file(jsonFilename);
    if (file.is_open()) {
        file << jBlockList.dump(2);  // 2 spaces for indentation
        file.close();
        cout << "Write to " << jsonFilename << endl;
    } else {
        cerr << "Failed to open the file." << endl;
    }
    */
}