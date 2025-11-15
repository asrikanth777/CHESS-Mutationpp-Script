#include <iostream>
#include <mutation++/mutation++.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <iomanip>
#include <filesystem>
namespace fs = std::filesystem;
using columns = std::pair<std::string, std::vector<double>>;
using table = std::vector<columns>;



table read_csv(std::string filename) {
	using namespace std;
	ifstream myFile(filename);

	if (!myFile.is_open()) {
	 throw runtime_error("Could not open file");
	}

	table result;
	string line, colname;

	if (getline(myFile,line)) {
	 cout << "File has been opened" << endl;
	 stringstream ss(line);
	 while (getline(ss, colname, ',')) {
	  result.emplace_back(colname, std::vector<double>{});
	 }
	}

	cout << "Headers: " ;
	for (const auto& col: result) {
	 cout << col.first << " , ";
	}
	cout << endl;

	while (getline(myFile, line)) {
	 stringstream ss(line);
	 double val;
	 size_t j = 0;

	 while (ss >> val) {

	  if (j < result.size()) {
	   result[j].second.push_back(val);
	  }

	  if (ss.peek() == ',') {
	   ss.ignore();
	  }
         j++;
	 }
	}
	return result;
}

static int index_of(const table& t, const std::string& name){
	for (size_t i = 0; i < t.size(); ++i) 
		if (t[i].first == name)
			return (int)i;
	return -1;
}




table mutationpp_calc(const table& in) {
	using namespace Mutation;
	using namespace Mutation::Thermodynamics;

	MixtureOptions opts("air_11");
	opts.setStateModel("EquilTP");
	opts.setThermodynamicDatabase("RRHO");
	Mixture mix(opts);
	mix.addComposition("N:0.79, O:0.21", true);

	const int iT = index_of(in, "T");
	const int iP = index_of(in, "p");
	if (iT < 0 || iP < 0) throw std::runtime_error("Missing columns temp or press");
	const auto& T = in[(size_t)iT].second;
	const auto& P = in[(size_t)iP].second;
	if (T.size() != P.size()) throw std::runtime_error("temp and press column lengths differ");

	std::vector<double> S;
	S.reserve(T.size());
	std::vector<std::vector<double>> Xvals(mix.nSpecies());
	for (auto& v: Xvals) v.reserve(T.size());

	for (size_t i = 0; i < T.size(); ++i) {
		double Ti = T[i];
		double Pi = P[i];
		mix.setState(&Ti, &Pi);

		S.push_back(mix.mixtureSMass());
		const double* X = mix.X();
		for (int j = 0; j < mix.nSpecies(); ++j)
			Xvals[j].push_back(X[j]);

	}


	table out = in;
	out.emplace_back(columns{"S", std::move(S)});
	for (int j = 0; j < mix.nSpecies(); ++j) {
		std::string col_name = "X_" + mix.speciesName(j);
		out.emplace_back(columns{col_name, std::move(Xvals[j])});
	}

	return out; 

}

static std::string csv_insurance(const std::string& str) {
	bool needs_quote = false;
	for (char c: str) {
		if (c == ',' || c == '"' || c == '\n' || c == '\r' || c == '\t' || c == ' '){
			needs_quote = true; break;
		}
	}
	if (!needs_quote) return str;
	std::string out; out.reserve(str.size() + 2);
	out.push_back('"');
	for (char c: str) {
		if (c == '"') out.push_back('"');
		out.push_back(c);
	}
	out.push_back('"');
	return out;
}

void write_csv(const std::string& path, const table& t, int precision = 10) {
	if (t.empty()) throw std::runtime_error("write_csv: empty table");

	const std::size_t nrows = t.front().second.size();
	for (const auto& col: t) {
		if (col.second.size() != nrows) {
			throw std::runtime_error("write_csv: column'" + col.first + "'has length" + std::to_string(col.second.size()) + " != " + std::to_string(nrows));
		}
	}
	std::ofstream f(path);
	if (!f) throw std::runtime_error("write_csv: cannot open '" + path + "'");

	for (std::size_t j = 0; j < t.size(); ++j) {
		f << csv_insurance(t[j].first);
		if (j + 1 < t.size()) f << ',';
	}
	f << '\n';

	f << std::setprecision(precision) << std::fixed;

	for (std::size_t i = 0; i < nrows; ++i){
		for (std::size_t j = 0; j < t.size(); ++j) {
			f << t[j].second[i];
			if (j + 1 < t.size()) f<< ',';
		}
		f << '\n';
	}
}

std::vector<std::string> find_csv_files_as_strings(const fs::path& directory_path) {
    std::vector<std::string> csv_file_names;

    try {
        // Use directory_iterator to walk the directory
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            // Check if the item is a regular file and has a .csv extension
            if (fs::is_regular_file(entry) && entry.path().extension() == ".csv") {
                // Convert the fs::path to a std::string and add it to the vector
                csv_file_names.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory " << directory_path << ": " << e.what() << std::endl;
    }

    return csv_file_names;
}





int main(int argc, char** argv) {

    // Get the current working directory as the target
    fs::path current_path = fs::current_path();

    // Call the function to get the vector of file names as strings
    std::vector<std::string> files_found = find_csv_files_as_strings(current_path);

    // Use the resulting vector in the main function
    if (files_found.empty()) {
        std::cout << "No .csv files found in the current directory." << std::endl;
    } else {
        std::cout << "Found .csv files:" << std::endl;
        for (const auto& file_path : files_found) {
            std::cout << "  - " << file_path << std::endl;
        }
    }

    for (size_t i = 0; i < files_found.size(); ++i) {
        // Convert the input string filename to a filesystem::path object
        fs::path input_path = files_found[i];

        // Construct the output filename
        fs::path output_path = input_path.parent_path() /
                               (input_path.stem().string() + "_output" + input_path.extension().string());
        
        // Read the input file
        table tbl = read_csv(input_path.string());
        
        // Perform the calculation
        table u = mutationpp_calc(tbl);
        
        // Write the result to the newly constructed output file
        write_csv(output_path.string(), u, 20);
    }
    
    return 0;
}

