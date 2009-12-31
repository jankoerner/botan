/*
* (C) 2009 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <iostream>
#include <botan/cpuid.h>

using namespace Botan;

void print_if_feature(const std::string& feature_name, bool exists)
   {
   if(exists)
      std::cout << feature_name << '\n';
   else
      std::cout << '[' << feature_name << ']' << '\n';
   }

int main()
   {
   std::cout << "Cache line size = " << CPUID::cache_line_size() << "\n";

   print_if_feature("RDTSC", CPUID::has_rdtsc());
   print_if_feature("SSE2", CPUID::has_sse2());
   print_if_feature("SSSE3", CPUID::has_ssse3());
   print_if_feature("SSE4.1", CPUID::has_sse41());
   print_if_feature("SSE4.2", CPUID::has_sse42());

   print_if_feature("AES-NI", CPUID::has_aes_intel());
   print_if_feature("AES-VIA", CPUID::has_aes_via());

   print_if_feature("AltiVec", CPUID::has_altivec());
   }
