/*
 * StorageTest.cpp
 *
 *  Created on: 27 Jan 2010
 *      Author: db434
 */

#include <gtest/gtest.h>
#include "Test.h"
#include "../Memory/AddressedStorage.h"
#include "../Memory/IPKCacheStorage.h"

//TEST(StorageTest, AddressedStorageWorks) {
//
//  AddressedStorage<int> as(64);
//
//  int a = 12;
//  int b = 1;
//  int c = 100;
//
//  as.write(&a, 0);
//  as.write(&b, 63);
//
//  EXPECT_EQ(a, *(as.read(0)));
//  EXPECT_EQ(b, *(as.read(63)));
//
//  as.write(&c, 0);
//
//  EXPECT_EQ(c, *(as.read(0)));
//
////  as.write(&c, 64);
//
//}


//TEST(StorageTest, IPKCacheStorageWorks) {
//
//  IPKCacheStorage<int, int> ipkc(64);
//
//  int a = 12;
//  int b = 3;
//  int c = 100;
//
//  ipkc.write(0, &a);
//  ipkc.write(63, &b);
//  ipkc.write(64, &c);
//
//  EXPECT_EQ(true, ipkc.checkTags(63));
//  EXPECT_EQ(true, ipkc.checkTags(64));
//  EXPECT_EQ(false, ipkc.checkTags(4));
//
//  EXPECT_EQ(a, *(ipkc.read()));
//  EXPECT_EQ(b, *(ipkc.read()));
//  EXPECT_EQ(c, *(ipkc.read()));
//
//  // Make this more thorough when we have instruction packets and stuff
//
//}
