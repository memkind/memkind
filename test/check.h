#ifndef check_include_h
#define check_include_h

#define PAGEMAP_ENTRY 8

class Check {
  public:
    int check_page_hbw(size_t num_bandwidth, const int *bandwidth, 
                     const void *ptr, size_t size);
    int check_page_size(void **ptr, size_t page_size,
                        int nr_pages);
  private:
    int is_page_present(void *vaddr, size_t page_size);


};

#endif
