#include "convert_raw_to_singles.h"
#include "FileType.h"

#include <RawReader.h>
#include <OrderedEventHandler.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>
#include <iostream>
#include <string>
#include <SystemConfig.h>
#include <CoarseSorter.h>
#include <ProcessHit.h>
#include <SimpleGrouper.h>
#include <CoincidenceGrouper.h>

#include <boost/lexical_cast.hpp>

#include <TFile.h>
#include <TTree.h>

using namespace std;
using namespace PETSYS;

//enum FILE_TYPE { FILE_TEXT, FILE_BINARY, FILE_ROOT, FILE_NULL };

class DataFileWriter {
private:
	std::string fName;
	double frequency;
	FILE_TYPE fileType;
	int eventFractionToWrite;
	long long eventCounter;
	double fileSplitTime;
	long long currentFilePartIndex;

	FILE *dataFile;
	FILE *indexFile;
	off_t stepBegin;
	
	TTree *hData;
	TTree *hIndex;
	TFile *hFile;
	// ROOT Tree fields
	float		brStep1;
	float		brStep2;
	long long 	brStepBegin;
	long long 	brStepEnd;

	long long	brTime;
	unsigned int	brChannelID;
	float		brToT;
	float		brEnergy;
	unsigned short	brTacID;
	int		brXi;
	int		brYi;
	float		brX;
	float 		brY;
	float 		brZ;
	float		brTQT;
	float		brTQE;
	

	struct SingleEvent {
		long long time;
		float e;
		int id;  
	} __attribute__((__packed__));
	
public:
	DataFileWriter(const char *fName, double frequency, FILE_TYPE fileType, int eventFractionToWrite, float splitTime) {
		this->fName = std::string(fName);
		this->frequency = frequency;
		this->fileType = (strcmp(fName, "/dev/null") != 0) ? fileType : FILE_NULL;
		this->eventFractionToWrite = eventFractionToWrite;
		this->eventCounter = 0;

		this->fileSplitTime = splitTime * frequency; // Convert from seconds to clock cycles
		this->currentFilePartIndex = 0;

		openFile();
	};

	void openFile() {
		stepBegin = 0;
		if (fileType == FILE_ROOT){
			hFile = new TFile(fName.c_str(), "RECREATE");
			int bs = 512*1024;

			hData = new TTree("data", "Event List", 2);
			hData->Branch("step1", &brStep1, bs);
			hData->Branch("step2", &brStep2, bs);
			hData->Branch("time", &brTime, bs);
			hData->Branch("channelID", &brChannelID, bs);
			hData->Branch("tot", &brToT, bs);
			hData->Branch("energy", &brEnergy, bs);
			hData->Branch("tacID", &brTacID, bs);
			hData->Branch("xi", &brXi, bs);
			hData->Branch("yi", &brYi, bs);
			hData->Branch("x", &brX, bs);
			hData->Branch("y", &brY, bs);
			hData->Branch("z", &brZ, bs);
			hData->Branch("tqT", &brTQT, bs);
			hData->Branch("tqE", &brTQE, bs);
			
			hIndex = new TTree("index", "Step Index", 2);
			hIndex->Branch("step1", &brStep1, bs);
			hIndex->Branch("step2", &brStep2, bs);
			hIndex->Branch("stepBegin", &brStepBegin, bs);
			hIndex->Branch("stepEnd", &brStepEnd, bs);
		}
		else if(fileType == FILE_BINARY) {
			char *fName2 = new char[1024];
			sprintf(fName2, "%s.ldat", fName.c_str());
			dataFile = fopen(fName2, "w");
			sprintf(fName2, "%s.lidx", fName.c_str());
			indexFile = fopen(fName2, "w");
			assert(dataFile != NULL);
			assert(indexFile != NULL);
			delete [] fName2;
		}
		else if (fileType == FILE_TEXT) {
			dataFile = fopen(fName.c_str(), "w");
			assert(dataFile != NULL);
			indexFile = NULL;
		}
	};
	
	~DataFileWriter() {
		closeFile();
		if(fileSplitTime > 0) {
			renameFile();
		}
	};

	void closeFile() {
		if (fileType == FILE_ROOT){
			hFile->Write();
			hFile->Close();
		}
		else if(fileType == FILE_BINARY) {
			fclose(dataFile);
			fclose(indexFile);
		}
		else if (fileType == FILE_TEXT) {
			fclose(dataFile);
		}
	};
	
	void closeStep(float step1, float step2) {
		if (fileType == FILE_ROOT){
			brStepBegin = stepBegin;
			brStepEnd = hData->GetEntries();
			brStep1 = step1;
			brStep2 = step2;
			hIndex->Fill();
			stepBegin = hData->GetEntries();
		}
		else if(fileType == FILE_BINARY) {
			fprintf(indexFile, "%ld\t%ld\t%e\t%e\n", stepBegin, ftell(dataFile), step1, step2);
			stepBegin = ftell(dataFile);
		}
		else {
			// Do nothing
		}
	};


	void renameFile() {
		char *fName1 = new char[1024];
		char *fName2 = new char[1024];
		if(fileType == FILE_BINARY) {
			// Binary output consists of two files and fName is their common prefix

			sprintf(fName1, "%s.ldat", fName.c_str());
			sprintf(fName2, "%s_%08lld.ldat", fName.c_str(), currentFilePartIndex);
			int r = rename(fName1, fName2);
			assert(r == 0);

			sprintf(fName1, "%s.lidx", fName.c_str());
			sprintf(fName2, "%s_%08lld.lidx", fName.c_str(), currentFilePartIndex);
			r = rename(fName1, fName2);
			assert(r == 0);

		}
		else {
			// ROOT or text output consists of a single file and fName is the complete fileName
			strcpy(fName1, fName.c_str());
			char *p = rindex(fName1, '.');

			if(p == NULL) {
				// If fName lacks a "." append the file part number at the end of the file name
				sprintf(fName2, "%s_%08lld", fName1, currentFilePartIndex);
				int r = rename(fName1, fName2);
				assert(r == 0);
			}
			else {
				// Insert the file part number before the extension
				char tmp = *p;
				*p = '\0';
				sprintf(fName2, "%s_%08lld.%s", fName1, currentFilePartIndex, p+1);
				*p = tmp;
				int r = rename(fName1, fName2);
				assert(r == 0);
			}

		}
		delete [] fName2;
		delete [] fName1;
	};
	
	void addEvents(float step1, float step2,EventBuffer<Hit> *buffer) {
		long long filePartIndex = (int)floor(buffer->getTMin() / fileSplitTime);

		if((fileSplitTime > 0) && (filePartIndex > currentFilePartIndex)) {
			closeStep(step1, step2);
			closeFile();
			renameFile();

			openFile();
			currentFilePartIndex = filePartIndex;
		}
		
		double Tps = 1E12/frequency;
		float Tns = Tps / 1000;
		long long tMin = buffer->getTMin() * (long long)Tps;

		int N = buffer->getSize();
		for (int i = 0; i < N; i++) {
			long long tmpCounter = eventCounter;
			eventCounter += 1;
			if((tmpCounter % 1024) >= eventFractionToWrite) continue;

			Hit &hit = buffer->get(i);
			if(!hit.valid) continue;

			float Eunit = hit.raw->qdcMode ? 1.0 : Tns;
			
			if (fileType == FILE_ROOT){
				brStep1 = step1;
				brStep2 = step2;
				
				brTime = ((long long)(hit.time * Tps)) + tMin;
				brChannelID = hit.raw->channelID;
				brToT = (hit.timeEnd - hit.time) * Tps;
				brEnergy = hit.energy * Eunit;
				brTacID = hit.raw->tacID;
				brTQT = hit.raw->time - hit.time;
				brTQE = (hit.raw->timeEnd - hit.timeEnd);
				brX = hit.x;
				brY = hit.y;
				brZ = hit.z;
				brXi = hit.xi;
				brYi = hit.yi;
				
				hData->Fill();
			}
			else if(fileType == FILE_BINARY) {
				SingleEvent eo = {
					((long long)(hit.time * Tps)) + tMin,
					hit.energy * Eunit,
					(int)hit.raw->channelID
				};
				fwrite(&eo, sizeof(eo), 1, dataFile);
			}
			else if (fileType == FILE_TEXT) {
				fprintf(dataFile, "%lld\t%f\t%d\n",
					((long long)(hit.time * Tps)) + tMin,
					hit.energy * Eunit,
					(int)hit.raw->channelID
					);
			}
		}
		
	}
	
};

class WriteHelper : public OrderedEventHandler<Hit, Hit> {
private: 
	DataFileWriter *dataFileWriter;
	float step1;
	float step2;
public:
	WriteHelper(DataFileWriter *dataFileWriter, float step1, float step2, EventSink<Hit> *sink) :
		OrderedEventHandler<Hit, Hit>(sink),
		dataFileWriter(dataFileWriter), step1(step1), step2(step2)
	{
	};
	
	EventBuffer<Hit> * handleEvents(EventBuffer<Hit> *buffer) {
		dataFileWriter->addEvents(step1, step2,buffer);
		return buffer;
	};
	
	void pushT0(double t0) { };
	void report() { };
};

static void displayHelp(char * program)
{
	fprintf(stderr, "Usage: %s --config <config_file> -i <input_file_prefix> -o <output_file_prefix> [optional arguments]\n", program);
	fprintf(stderr, "Arguments:\n");
	fprintf(stderr,  "  --config \t\t Configuration file containing path to tdc calibration table \n");
	fprintf(stderr,  "  -i \t\t\t Input file prefix - raw data\n");
	fprintf(stderr,  "  -o \t\t\t Output file name - by default in text dataformat\n");
	fprintf(stderr, "Optional flags:\n");
	fprintf(stderr,  "  --writeBinary \t Set the output data format to binary\n");
	fprintf(stderr,  "  --writeRoot \t\t Set the output data format to ROOT TTree\n");
	fprintf(stderr,  "  --writeFraction N \t\t Fraction of events to write. Default: 100%%.\n");
	fprintf(stderr,  "  --splitTime t \t\t Split output into different files every t seconds.\n");
	fprintf(stderr,  "  --help \t\t Show this help message and exit \n");	
	
};

static void displayUsage(char *argv0)
{
	printf("Usage: %s --config <config_file> -i <input_file_prefix> -o <output_file_prefix> [optional arguments]\n", argv0);
}

bool runConvertRawToSingles(const std::string& configFileName,
                            const std::string& inputFilePrefix,
                            const std::string& outputFileName,
                            FILE_TYPE fileType,
                            long long eventFractionToWrite,
                            double fileSplitTime)
{
  if (configFileName.empty() || inputFilePrefix.empty() || outputFileName.empty()) {
    //cerr << "Error: config, input, and output arguments are mandatory." << endl;
    //return false;
    std::ostringstream oss;
    oss << "Error: config, input, and output arguments are mandatory.";
    throw std::runtime_error(oss.str());
  }

	if(configFileName == NULL) {
		//fprintf(stderr, "--config must be specified\n");
		//exit(1);
    std::ostringstream oss;
    oss << "ERROR: --config must be specified";
    throw std::runtime_error(oss.str());
	}
	
	if(inputFilePrefix == NULL) {
		//fprintf(stderr, "-i must be specified\n");
		//exit(1);
    std::ostringstream oss;
    oss << "ERROR: -i must be specified";
    throw std::runtime_error(oss.str());
	}

	if(outputFileName == NULL) {
		//fprintf(stderr, "-o must be specified\n");
		//exit(1);
    std::ostringstream oss;
    oss << "ERROR: -o must be specified";
    throw std::runtime_error(oss.str());
	}

	RawReader *reader = RawReader::openFile(inputFilePrefix.c_str());
	
	// If data was taken in ToT mode, do not attempt to load these files
	unsigned long long mask = SystemConfig::LOAD_ALL;
	if(reader->isTOT()) {
		mask ^= (SystemConfig::LOAD_QDC_CALIBRATION | SystemConfig::LOAD_ENERGY_CALIBRATION);
	}
	SystemConfig *config = SystemConfig::fromFile(configFileName.c_str(), mask);
	
	DataFileWriter *dataFileWriter = new DataFileWriter(outputFileName.c_str(), reader->getFrequency(),  fileType, eventFractionToWrite, fileSplitTime);
	
	int stepIndex = 0;
	while(reader->getNextStep()) {
		float step1, step2;
		reader->getStepValue(step1, step2);
		printf("Processing step %d: (%f, %f)\n", stepIndex+1, step1, step2);
		fflush(stdout);
		reader->processStep(true,
				new CoarseSorter(
				new ProcessHit(config, reader,
				new WriteHelper(dataFileWriter, step1, step2,
				new NullSink<Hit>()
				))));
		
		dataFileWriter->closeStep(step1, step2);
		stepIndex += 1;
	}

	delete dataFileWriter;
	delete reader;

	return true;
}

