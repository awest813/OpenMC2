//***************************************************************************
//
//	PQueue.h -- Prototype for priority queues
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef PQUEUE_H
#define PQUEUE_H

#include <stdint.h>
#include <unordered_map>

//***************************************************************************

//--------------
// Include Files

//--------------------------------
// Structure and Class Definitions

typedef struct _PQNode {
	int32_t         key;			// sort value
	uint32_t        id;				// hash value for this map position
	int32_t         row;			// HB-specific
	int32_t         col;			// HB-specific
} PQNode;

class PriorityQueue {

	protected:

		PQNode*		pqList;
		int32_t     maxItems;
		int32_t     numItems;
		int32_t     keyMin;
		// heap index of each id in the queue, so find() doesn't scan the whole queue (pathfinding
		// calls it for every decrease-key); ids in the queue are unique
		std::unordered_map<uint32_t, int> indexOf;

		// stores a node at a heap index and records where its id is
		void place (int index, const PQNode& node) {
			pqList[index] = node;
			indexOf[node.id] = index;
		}

		void downHeap (int curIndex);

		void upHeap (int curIndex);

	public:

		void init (void) {
			pqList = NULL;
			numItems = 0;
		}

		PriorityQueue (void) {
			init();
		}

		int init (int maxItems, int keyMinValue = -2000000);

		int insert (PQNode& item);

		void remove (PQNode& item);

		void change (int itemIndex, int newValue);

		int find (unsigned int id);

		int findByKey (int32_t key, uint32_t id, int startIndex = 1);
		
		void clear (void) {
			numItems = 0;
			indexOf.clear();
		}

		int getNumItems (void) { return(numItems); }
		
		bool isEmpty (void) {
			return(numItems == 0);
		}
		
		PQNode* getItem (int itemIndex) {
			return(&pqList[itemIndex]);
		}

		void destroy (void);

		~PriorityQueue (void) {
			destroy();
		}
};

typedef PriorityQueue* PriorityQueuePtr;

//***************************************************************************

#endif
