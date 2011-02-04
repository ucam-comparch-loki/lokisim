//-------------------------------------------------------------------------------------------------
// Loki Project
// Software Simulator for Design Space Exploration
//-------------------------------------------------------------------------------------------------
// Batch Mode Event Recorder Definition
//-------------------------------------------------------------------------------------------------
// Defines an aggregating event recorder with roll-back capability to record delta cycle safe
// statistics of events across the simulation model. The recorded data can be written in machine
// readable form to a file for automated processing.
//-------------------------------------------------------------------------------------------------
// File:       BatchModeEventRecorder.h
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 03/02/2011
//-------------------------------------------------------------------------------------------------

#ifndef BATCHMODEEVENTRECORDER_H_
#define BATCHMODEEVENTRECORDER_H_

class BatchModeEventRecorder {
private:
	static const int kInstanceHashBits = 10;		// At most 16 bits
	static const int kEventBufferEntries = 256;

	struct PropertyData {
	public:
		PropertyData *ListNext;

		int PropertyType;
		long long PropertyValue;
	};

	struct InstanceData {
	public:
		InstanceData *ListNext;
		InstanceData *HashNext;

		void *InstancePtr;
		int InstanceType;

		PropertyData *PropertyList;
		int PropertyCount;

		int EventFrequenciesPending[kEventBufferEntries];
		long long EventFrequenciesCommitted[kEventBufferEntries];
	};

#pragma pack(push)
#pragma pack(1)

	struct FileHeader {
	public:
		static const long long kSignature = 0x1234567887654321LL;

		long long Signature;
		long long CreationTime;
		long long GlobalPropertyCount;
		long long InstanceCount;
	};

	struct FileFooter {
	public:
		static const long long kSignature = 0x8765432112345678LL;

		long long Signature;
		long long BytesWritten;
	};

	struct FileProperty {
	public:
		long long PropertyType;
		long long PropertyValue;
	};

	struct FileInstance {
	public:
		long long InstanceType;
		long long PropertyCount;
		long long EventCount;
	};

#pragma pack(pop)

	InstanceData *mInstanceList;
	int mInstanceCount;

	PropertyData *mGlobalPropertyList;
	int mGlobalPropertyCount;

	InstanceData **mInstanceHashTable;

	InstanceData* getInstanceData(void *instancePtr) const;
public:
	static const int kInstanceSharedL1CacheSubsystem				= 1000;
	static const int kInstanceSharedL1CacheNetworkInterface			= 1010;
	static const int kInstanceSharedL1CacheController				= 1011;
	static const int kInstanceSharedL1CacheCrossbarSwitch			= 1012;
	static const int kInstanceSharedL1CacheBank						= 1013;
	static const int kInstanceSimplifiedBackgroundMemory			= 1100;

	static const int kPropertySharedL1CacheChannels					= 1000;
	static const int kPropertySharedL1CacheInterfaceQueueDepth		= 1001;
	static const int kPropertySharedL1CacheBanks					= 1002;
	static const int kPropertySharedL1CacheSetsPerBank				= 1003;
	static const int kPropertySharedL1CacheAssociativity			= 1004;
	static const int kPropertySharedL1CacheLineSize					= 1005;
	static const int kPropertySharedL1CacheSequentialSearch			= 1006;
	static const int kPropertySharedL1CacheRandomReplacement		= 1007;
	static const int kPropertySharedL1CacheMemoryQueueDepth			= 1008;
	static const int kPropertySharedL1CacheMemoryDelayCycles		= 1009;

	static const int kPropertySharedL1CacheChannelNumber			= 1050;
	static const int kPropertySharedL1CacheBankNumber				= 1051;

	BatchModeEventRecorder();
	~BatchModeEventRecorder();

	void setGlobalProperty(int propertyType, long long propertyValue);

	void registerInstance(void *instancePtr, int instanceType);
	void setInstanceProperty(void *instancePtr, int propertyType, long long propertyValue);

	void resetInstanceEvents(void *instancePtr);
	void commitInstanceEvents(void *instancePtr);
	void recordInstanceEvent(void *instancePtr, int eventType);

	void storeStatisticsToFile(FILE *outputFile);
};

#endif /* BATCHMODEEVENTRECORDER_H_ */
