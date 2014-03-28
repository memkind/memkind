#ifndef check_include_h
#define check_include_h

class Check {
  public:
    int check_page_hbw(size_t num_bandwidth, const int *bandwidth, 
                     const void *ptr, size_t size);

    int check_page_size(void *ptr, size_t size, size_t page_size);
};

#endif
