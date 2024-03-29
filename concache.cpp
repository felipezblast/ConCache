//Felipe Viana 3/27/2024

#include <iostream>

#include <map>

#include <memory>

#include <tuple>

#include <functional>

#include <mutex>

#include <list>

#include <future>

template < typename...Args >
   using TupleKey = std::tuple < Args... > ;

template < typename ReturnType, typename...Args >
   class FunctionCache {
      public: FunctionCache(std:: function < ReturnType(Args...) > func, std::size_t maxSize = 100): func(func),
      maxSize(maxSize) {}

      std::future < ReturnType > get(Args...args) {
         std::unique_lock < std::mutex > lock(cacheMutex);
         TupleKey < Args... > key = std::make_tuple(args...);
         auto it = cacheMap.find(key);
         if (it != cacheMap.end()) {
            usageList.splice(usageList.begin(), usageList, it -> second.second);
            return std::async (std::launch::deferred, [it] {
               return it -> second.first;
            });
         } else {
            lock.unlock();
            ReturnType result = func(args...);
            lock.lock();
            if (cacheMap.size() == maxSize) {
               auto last = usageList.back();
               cacheMap.erase(last);
               usageList.pop_back();
            }
            usageList.push_front(key);
            cacheMap[key] = {
               result,
               usageList.begin()
            };

            return std::async (std::launch::deferred, [result] {
               return result;
            });
         }
      }

      private: std::map < TupleKey < Args... > ,
      std::pair < ReturnType,
      typename std::list < TupleKey < Args... >> ::iterator >> cacheMap;
      std::list < TupleKey < Args... >> usageList;
      std:: function < ReturnType(Args...) > func;
      std::mutex cacheMutex;
      std::size_t maxSize;
   };

int expensiveFunction(int x) {
   for (int i = 0; i < 1000000; i++) {
      x += i % 10;
   }
   return x;
}

int main() {
   FunctionCache < int, int > cache(expensiveFunction, 2);
   auto resultFuture1 = cache.get(10);
   std::cout << "Result: " << resultFuture1.get() << std::endl; // Computed
   auto resultFuture2 = cache.get(10);
   std::cout << "Result: " << resultFuture2.get() << std::endl; // Cached
   auto resultFuture3 = cache.get(20);
   std::cout << "Result: " << resultFuture3.get() << std::endl; // Computed and triggers eviction if cache size exceeds limit
   return 0;
}
