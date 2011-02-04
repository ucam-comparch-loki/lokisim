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
// File:       BatchModeEventRecorder.cpp
// Author:     Andreas Koltes (andreas.koltes@cl.cam.ac.uk)
// Created on: 03/02/2011
//-------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "BatchModeEventRecorder.h"

BatchModeEventRecorder::InstanceData* BatchModeEventRecorder::getInstanceData(void *instancePtr) const {
	unsigned long long num = (unsigned long long)instancePtr;
	unsigned long hash = (num >> 32) ^ num;
	hash = (hash >> 16) ^ hash;
	hash &= (1UL << kInstanceHashBits) - 1UL;

	InstanceData *instCursor = mInstanceHashTable[hash];
	while (instCursor != NULL) {
		if (instCursor->InstancePtr == instancePtr)
			return instCursor;
	}

	fprintf(stderr, "Instance not found - call registerInstance() before calling any other method on an instance pointer\n");
	exit(1);
}

BatchModeEventRecorder::BatchModeEventRecorder() {
	mInstanceList = NULL;
	mInstanceCount = 0;

	mGlobalPropertyList = NULL;
	mGlobalPropertyCount = 0;

	mInstanceHashTable = new InstanceData*[1 << kInstanceHashBits];

	memset(mInstanceHashTable, 0x00, sizeof(InstanceData*) * (1 << kInstanceHashBits));
}

BatchModeEventRecorder::~BatchModeEventRecorder() {
	InstanceData *instCursor = mInstanceList;
	while (instCursor != NULL) {
		InstanceData *inst = instCursor;

		PropertyData *propCursor = inst->PropertyList;
		while (propCursor != NULL) {
			PropertyData *prop = propCursor;
			propCursor = propCursor->ListNext;
			delete prop;
		}

		instCursor = inst->ListNext;
		delete inst;
	}

	PropertyData *propCursor = mGlobalPropertyList;
	while (propCursor != NULL) {
		PropertyData *prop = propCursor;
		propCursor = prop->ListNext;
		delete prop;
	}

	delete[] mInstanceHashTable;
}

void BatchModeEventRecorder::setGlobalProperty(int propertyType, long long propertyValue) {
	PropertyData *propCursor = mGlobalPropertyList;
	while (propCursor != NULL) {
		if (propCursor->PropertyType == propertyType) {
			propCursor->PropertyValue = propertyValue;
			return;
		}

		propCursor = propCursor->ListNext;
	}

	PropertyData *newProperty = new PropertyData;
	newProperty->ListNext = mGlobalPropertyList;
	newProperty->PropertyType = propertyType;
	newProperty->PropertyValue = propertyValue;

	mGlobalPropertyList = newProperty;
	mGlobalPropertyCount++;
}

void BatchModeEventRecorder::registerInstance(void *instancePtr, int instanceType) {
	unsigned long long num = (unsigned long long)instancePtr;
	unsigned long hash = (num >> 32) ^ num;
	hash = (hash >> 16) ^ hash;
	hash &= (1UL << kInstanceHashBits) - 1UL;

	InstanceData *newInstance = new InstanceData;
	newInstance->ListNext = mInstanceList;
	newInstance->HashNext = mInstanceHashTable[hash];
	newInstance->InstancePtr = instancePtr;
	newInstance->InstanceType = instanceType;
	newInstance->PropertyList = NULL;
	newInstance->PropertyCount = 0;

	memset(newInstance->EventFrequenciesPending, 0x00, sizeof(newInstance->EventFrequenciesPending));
	memset(newInstance->EventFrequenciesCommitted, 0x00, sizeof(newInstance->EventFrequenciesCommitted));

	mInstanceList = newInstance;
	mInstanceHashTable[hash] = newInstance;
	mInstanceCount++;
}

void BatchModeEventRecorder::setInstanceProperty(void *instancePtr, int propertyType, long long propertyValue) {
	InstanceData *instData = getInstanceData(instancePtr);

	PropertyData *propCursor = instData->PropertyList;
	while (propCursor != NULL) {
		if (propCursor->PropertyType == propertyType) {
			propCursor->PropertyValue = propertyValue;
			return;
		}

		propCursor = propCursor->ListNext;
	}

	PropertyData *newProperty = new PropertyData;
	newProperty->ListNext = instData->PropertyList;
	newProperty->PropertyType = propertyType;
	newProperty->PropertyValue = propertyValue;

	instData->PropertyList = newProperty;
	instData->PropertyCount++;
}

void BatchModeEventRecorder::resetInstanceEvents(void *instancePtr) {
	InstanceData *instData = getInstanceData(instancePtr);
	memset(instData->EventFrequenciesPending, 0x00, sizeof(instData->EventFrequenciesPending));
}

void BatchModeEventRecorder::commitInstanceEvents(void *instancePtr) {
	InstanceData *instData = getInstanceData(instancePtr);

	for (int i = 0; i < kEventBufferEntries; i++)
		instData->EventFrequenciesCommitted[i] += instData->EventFrequenciesPending[i];

	memset(instData->EventFrequenciesPending, 0x00, sizeof(instData->EventFrequenciesPending));
}

void BatchModeEventRecorder::recordInstanceEvent(void *instancePtr, int eventType) {
	InstanceData *instData = getInstanceData(instancePtr);
	instData->EventFrequenciesPending[eventType]++;
}

void BatchModeEventRecorder::storeStatisticsToFile(FILE *outputFile) {
	long long bytesWritten = 0;

	// File header

	FileHeader header;
	header.Signature = FileHeader::kSignature;
	header.CreationTime = (long long)time(NULL);
	header.GlobalPropertyCount = mGlobalPropertyCount;
	header.InstanceCount = mInstanceCount;
	fwrite(&header, 1, sizeof(header), outputFile);
	bytesWritten += sizeof(header);

	// Global properties

	PropertyData *propCursor = mGlobalPropertyList;
	while (propCursor != NULL) {
		FileProperty property;
		property.PropertyType = propCursor->PropertyType;
		property.PropertyValue = propCursor->PropertyValue;
		fwrite(&property, 1, sizeof(property), outputFile);
		bytesWritten += sizeof(property);

		propCursor = propCursor->ListNext;
	}

	// Instances

	InstanceData *instCursor = mInstanceList;
	while (instCursor != NULL) {
		// Instance header

		FileInstance instance;
		instance.InstanceType = instCursor->InstanceType;
		instance.PropertyCount = instCursor->PropertyCount;
		instance.EventCount = kEventBufferEntries;
		fwrite(&instance, 1, sizeof(instance), outputFile);
		bytesWritten += sizeof(instance);

		// Instance properties

		PropertyData *propCursor = instCursor->PropertyList;
		while (propCursor != NULL) {
			FileProperty property;
			property.PropertyType = propCursor->PropertyType;
			property.PropertyValue = propCursor->PropertyValue;
			fwrite(&property, 1, sizeof(property), outputFile);
			bytesWritten += sizeof(property);

			propCursor = propCursor->ListNext;
		}

		// Event frequencies

		fwrite(instCursor->EventFrequenciesCommitted, 1, sizeof(instCursor->EventFrequenciesCommitted), outputFile);
		bytesWritten += sizeof(instCursor->EventFrequenciesCommitted);

		instCursor = instCursor->ListNext;
	}

	// File footer

	FileFooter footer;
	footer.Signature = FileFooter::kSignature;
	footer.BytesWritten = bytesWritten;
	fwrite(&footer, 1, sizeof(footer), outputFile);
}
