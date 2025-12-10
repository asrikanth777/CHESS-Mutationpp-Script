#include <iostream>
#include <mutation++/mutation++.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
// #include <cmath>                   
// #include <unordered_map>				
// #include <utility>					
#include <stdexcept>
#include <iomanip>
#include <filesystem>


// creates a table by a vector of vectors, first value is a string
// for headers like x_O or lambda, the vector stores numeric values
using columns = std::pair<std::string, std::vector<double>>;
using table = std::vector<columns>;

// Global mixture settings, can be changed to use air_11, air_5, etc.
Mutation::Mixture make_air_mixture() {
    Mutation::MixtureOptions opts("air_11");
    opts.setStateModel("EquilTP");
    opts.setThermodynamicDatabase("RRHO");

    Mutation::Mixture m(opts);
    m.addComposition("N:0.79, O:0.21", true);
    return m;
}

// makes the mixture to be used
Mutation::Mixture mix = make_air_mixture();


// this function is used to parse the csv imported and return a table with csv content
table read_csv(std::string filename) {

	using namespace std;

	// opens csv and checks to see if it opens
	ifstream myFile(filename);

	if (!myFile.is_open()) {
	 throw runtime_error("Could not open file");
	}

	// sets up the table to store values
	table result;
	string line, colname;

	// this gets all the headers and appends them to first column
	if (getline(myFile,line)) {
	 cout << "File has been opened" << endl;
	 stringstream ss(line);
	 while (getline(ss, colname, ',')) {
	  result.emplace_back(colname, std::vector<double>{});
	 }
	}

	// restates headers in case u imported wrong csv (paraview can be iffy sometimes)
	cout << "Headers: " ;
	for (const auto& col: result) {
	 cout << col.first << " , ";
	}
	cout << endl;

	// this part then gets the values in corresponding columns of the csv and maps back to paraview
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

// function to loop through columns to find header matching name in function parameter
static int index_of(const table& t, const std::string& name){
	for (size_t i = 0; i < t.size(); ++i) 
		if (t[i].first == name)
			return (int)i;
	return -1;
}




// calculates mole fractions of flow species with T and P using mutation++
void mole_fraction(table& in) {
	using namespace Mutation;
	using namespace Mutation::Thermodynamics;

	// gets index of temperature and pressure columns
	// checks if they are empty or mismatched size (bad)
	const int iT = index_of(in, "T");
	const int iP = index_of(in, "p");
	if (iT < 0 || iP < 0) throw std::runtime_error("Missing columns temp or press");
	const auto& T = in[(size_t)iT].second;
	const auto& P = in[(size_t)iP].second;
	if (T.size() != P.size()) throw std::runtime_error("temp and press column lengths differ");

	const size_t N = T.size();

	// sets up Xvals table to track mole fractions
	std::vector<std::vector<double>> Xvals(mix.nSpecies());
	for (auto& v: Xvals) v.reserve(N);

	// loops through and does the calc
	for (size_t i = 0; i < N; ++i) {
		double Ti = T[i];
		double Pi = P[i];

		// uses reference instead of value to avoid copying
		mix.setState(&Ti, &Pi);

		// calculates entropy and mole fractions and puts them in
		// their respective columns
		const double* X = mix.X();
		for (int j = 0; j < mix.nSpecies(); ++j)
			Xvals[j].push_back(X[j]);

	}

	// overwrites table with new mole fraction calculations
	for (int j = 0; j < mix.nSpecies(); ++j) {
		std::string col_name = "X_" + mix.speciesName(j);
		in.emplace_back(columns{col_name, std::move(Xvals[j])});
	}

	// returns table

}

void entropy(table& in) {
	using namespace Mutation;
	using namespace Mutation::Thermodynamics;

	// gets index of temperature and pressure columns
	// checks if they are empty or mismatched size (bad)
	const int iT = index_of(in, "T");
	const int iP = index_of(in, "p");
	if (iT < 0 || iP < 0) throw std::runtime_error("Missing columns temp or press");
	const auto& T = in[(size_t)iT].second;
	const auto& P = in[(size_t)iP].second;
	if (T.size() != P.size()) throw std::runtime_error("temp and press column lengths differ");

	const size_t N = T.size();
	// sets up S vector for entropy generation
	std::vector<double> S;
	S.reserve(N);

	// loops through and does the calc
	for (size_t i = 0; i < N; ++i) {
		double Ti = T[i];
		double Pi = P[i];

		mix.setState(&Ti, &Pi);
		S.push_back(mix.mixtureSMass());

	}

	// appends table with new entropy column
	in.emplace_back(columns{"S", std::move(S)});

}

// This function takes temp and pressure and finds thermal conductivity
void thermal(table& in) {
	using namespace Mutation;
	using namespace Mutation::Thermodynamics;
	// same namespaces as before but using the transport.h header now
	using namespace Mutation::Transport;

	// leave the code below, maybe add to it if needing other values like velocity and stuff
	const int iT = index_of(in, "T");
	const int iP = index_of(in, "p");
	if (iT < 0 || iP < 0) throw std::runtime_error("Missing columns temp or press");
	const auto& T = in[(size_t)iT].second;
	const auto& P = in[(size_t)iP].second;
	if (T.size() != P.size()) throw std::runtime_error("temp and press column lengths differ");

	const size_t N = T.size();
	// sets up lambda table to store conductivity
	std::vector<double> lambda;
	lambda.reserve(N);

	// calculates lambda and appends vector
	for (size_t i = 0; i < N; ++i) {
		double Ti = T[i];
		double Pi = P[i];
		mix.setState(&Ti, &Pi);

		lambda.push_back(mix.equilibriumThermalConductivity());
	}


	// appends table with new vector
	in.emplace_back(columns{"lambda", std::move(lambda)});


}


// this makes sure that when making  csv, it doesn't keep quotations unnecessarily while not 
// letting escape characters through to make headers 
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

// this function writes to the csv and makes sure its a complete rectangular grid
void write_csv(const std::string& path, const table& t, int precision = 10) {
	// ensures table is not empty and columns are same size
	if (t.empty()) throw std::runtime_error("write_csv: empty table");
	const std::size_t nrows = t.front().second.size();
	for (const auto& col: t) {
		if (col.second.size() != nrows) {
			throw std::runtime_error("write_csv: column'" + col.first + "'has length" + std::to_string(col.second.size()) + " != " + std::to_string(nrows));
		}
	}
	// makes sure a valid output path is given, 
	std::ofstream f(path);
	if (!f) throw std::runtime_error("write_csv: cannot open '" + path + "'");

	// then this starts writing the rows into it, putting headers first
	for (std::size_t j = 0; j < t.size(); ++j) {
		f << csv_insurance(t[j].first);
		if (j + 1 < t.size()) f << ',';
	}
	f << '\n';

	// fixes precision of values instead of scientific notation
	f << std::setprecision(precision) << std::fixed;

	for (std::size_t i = 0; i < nrows; ++i){
		for (std::size_t j = 0; j < t.size(); ++j) {
			f << t[j].second[i];
			if (j + 1 < t.size()) f<< ',';
		}
		f << '\n';
	}
}

// this finds all the csv's in the same directory as script, and makes a list
std::vector<std::string> find_csv_files_as_strings(const std::filesystem::path& directory_path) {
	namespace fs = std::filesystem;
    std::vector<std::string> csv_file_names;

	// checks to see if there are any csv files in directory and appends filename vector accordingly
    try {

        for (const auto& entry : fs::directory_iterator(directory_path)) {

            if (fs::is_regular_file(entry) && entry.path().extension() == ".csv") {

                csv_file_names.push_back(entry.path().string());
            }
        }
	
		// makes sure directory path is valid
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory " << directory_path << ": " << e.what() << std::endl;
    }

    return csv_file_names;
}





int main() {
    namespace fs = std::filesystem;

    // take current path of where the binary is being run
    fs::path current_path = fs::current_path();

    // explicitly define input and output directories under the repo
    fs::path input_dir  = current_path / "input";
    fs::path output_dir = current_path / "output";

    // finds csv strings in the input directory
    std::vector<std::string> files_found = find_csv_files_as_strings(input_dir);

    // makes sure there are csv files to go through
    if (files_found.empty()) {
        std::cout << "No .csv files found in input directory: " << input_dir << std::endl;
    } else {
        std::cout << "Found .csv files in " << input_dir << ":" << std::endl;
        // shows what files are now being worked with
        for (const auto& file_path : files_found) {
            std::cout << "  - " << file_path << std::endl;
        }
    }

    // this part sets it up to add _output to differentiate from original csvs
    for (size_t i = 0; i < files_found.size(); ++i) {
        fs::path input_path = files_found[i];

        // put outputs into the output/ directory, not next to the inputs
        fs::path output_path = output_dir /
            (input_path.stem().string() + "_output" + input_path.extension().string());

        // Reads CSV, DO NOT COMMENT OUT
        table tbl = read_csv(input_path.string());

        // Mutation++ calculations, comment out whatever you dont need
        mole_fraction(tbl);
        // entropy(tbl);
        thermal(tbl);

        // Outputs table to csv, DO NOT COMMENT OUT
        write_csv(output_path.string(), tbl, 15);
    }

    return 0;
}

// thats the end so far

// room for improvements
// -----------------------------------------------------------------------------------------------------------
// + making it cache the csvs in a separate folder instead of populating current folder
//	- this is because if u run the script without clearing the old ones, it also takes the outputcsvs again 
// 	  and does the computation


