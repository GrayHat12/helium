#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <string>

#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./assembly.hpp"
#include "./arena.hpp"

std::string readFile(char *filepath)
{
    std::string contents;
    {
        std::stringstream codestream;
        std::fstream input(filepath, std::ios::in);
        codestream << input.rdbuf();
        contents = codestream.str();
    }
    return contents;
}

void writeFile(const std::string& filepath, const std::string *data)
{
    std::fstream output(filepath, std::ios::out);
    output << data->c_str();
}

struct File
{
    std::string name;
    std::optional<std::string> extn;
};

struct PathSplit
{
    std::string path;
    File file;
};

File filename_split(const std::string &filename)
{
    std::size_t found = filename.find_last_of('.');
    if (found == std::string::npos)
    {
        return {.name = filename};
    }
    else
    {
        return {.name = filename.substr(0, found), .extn = filename.substr(found + 1)};
    }
}

PathSplit path_split(const std::string &fullpath)
{
    std::size_t found = fullpath.find_last_of("/\\");
    if (found == std::string::npos)
    {
        return {.path = "", .file = filename_split(fullpath)};
    }
    else
    {
        return {.path = fullpath.substr(0, found), .file = filename_split(fullpath.substr(found + 1))};
    }
}

std::string generate_path(const PathSplit &path)
{
    std::stringstream out;
    std::string spath = path.path;
    std::stringstream exten;
    if (path.file.extn.has_value())
    {
        exten << "." << path.file.extn.value();
    }
    else
    {
        exten << "";
    }
    if (!spath.empty())
    {
        spath.push_back('/');
    }
    out << spath << path.file.name << exten.str();
    return out.str();
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Incorrect Usage" << std::endl;
        std::cerr << "Usage: `helium <filepath.he> <outfile>`" << std::endl;
        return EXIT_FAILURE;
    }

    Tokenizer tokenizer(readFile(argv[1]));

    std::vector<Token> tokens = tokenizer.tokenize();
    // for (Token token : tokens)
    // {
    //     std::cout << token.type << " : " << token.value.value_or("") << std::endl;
    // }
    ArenaAllocator allocator(1024 * 1024 * 4);
    Parser parser(tokens, &allocator);

    std::optional<Node::Program> prog_node = parser.parse();

    // std::cout << prog_node.value().to_string().str() << std::endl;

    if (!prog_node.has_value())
    {
        std::cerr << "ya messed up ya twat" << std::endl;
        exit(EXIT_FAILURE);
    }

    AssGenerator generator(prog_node.value(), &allocator);

    std::string asmcode = generator.generate_program();

    std::cout << prog_node.value().to_string().str() << std::endl;

    // std::cout << asmcode.str() << std::endl;

    PathSplit outFile = path_split(argv[2]);
    PathSplit asmFile = outFile;
    PathSplit objFile = outFile;
    asmFile.file.extn = "asm";
    objFile.file.extn = "o";

    std::string asmPath = generate_path(asmFile);
    std::string objPath = generate_path(objFile);
    std::string outPath = generate_path(outFile);

    writeFile(asmPath, &asmcode);

    system(("nasm -felf64 " + asmPath + " -o " + objPath).c_str());
    system(("ld -o " + outPath + " " + objPath).c_str());
    // system(("rm " + asmPath).c_str());
    system(("rm " + objPath).c_str());

    return EXIT_SUCCESS;
}