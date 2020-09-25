

#include <iostream>
#include <stdio.h>
#include <stdlib.h>


enum { _ALIGN = 8 };  // С��������ϵ��߽�
enum { _MAX_BYTES = 128 }; // С�����������
enum { _NFREELISTS = 16 }; // _MAX_BYTES/_ALIGN  free-list �ĸ���


static char* _S_start_free;  // �ڴ����ʼλ�á�ֻ�� _S_chunk_alloc() �б仯
static char* _S_end_free;    // �ڴ�ؽ���λ�á�ֻ�� _S_chunk_alloc() �б仯
static size_t _S_heap_size;

union _Obj {
	union _Obj* _M_free_list_link;  // �����������ص�
	char _M_client_data[1]; 
};

//��������
static _Obj*  _S_free_list[_NFREELISTS];

//��ȡ�ȴ���������8�ı�������С�����ڴ�ش���ڴ水��8�ı���Ϊһ����
// ��������7 byte ���ڿ�Ϊ8�������У�����17byte ���ڿ�Ϊ24��������
static size_t _S_round_up(size_t __bytes)
{
	return  (((__bytes)+(size_t)_ALIGN - 1) & ~((size_t)_ALIGN - 1));

}


//��ȡ �����Ӧ��С�ֽ� ��Ӧ�� _S_free_list �����±�
//_S_free_list ��һ����СΪ16�����飬ÿһ���±갴��8�ı��������Ӧ�Ŀ�
//�±�Ϊ0 ��� 8 byte�Ŀ������±�Ϊ1���16 byte���С.....
static  size_t _S_freelist_index(size_t __bytes) {

	return (((__bytes)+(size_t)_ALIGN - 1) / (size_t)_ALIGN - 1);
}

//�˺��������ڴ�ʹ�ã����ú󷵻�size�ĵ�ַ ��
/*
	_S_start_free ��ʾ�����ڴ����ʼ��ַ
	_S_end_free   ��ʾ������ڴ�Ľ�����ַ
	__result ʹ�õ��ڴ���ʼ��ַ

*/
char*_S_chunk_alloc(size_t __size,int& __nobjs)
{
	char* __result;
	size_t __total_bytes = __size * __nobjs;  // ��Ҫ����ռ�Ĵ�С 
	size_t __bytes_left = _S_end_free - _S_start_free;  // �����ڴ��ʣ��ռ�

	if (__bytes_left >= __total_bytes) {  // �ڴ��ʣ��ռ���ȫ��������
		__result = _S_start_free;
		_S_start_free += __total_bytes;
		return(__result);
	}
	else if (__bytes_left >= __size) {  // �ڴ��ʣ��ռ䲻���������룬�ṩһ�����ϵ�����

		__nobjs = (int)(__bytes_left / __size);

		__total_bytes = __size * __nobjs;

		__result = _S_start_free;

		_S_start_free += __total_bytes;

		return(__result);
	}
	else {       
		// �ڴ��ʣ��ռ���һ������Ĵ�С���޷��ṩ                      
		size_t alloc_bytes =2 * __total_bytes + _S_round_up(_S_heap_size >> 4);

		// �ڴ�ص�ʣ��ռ�ָ����ʵĿ�������
		if (__bytes_left > 0) {

			_Obj* * __my_free_list =_S_free_list + _S_freelist_index(__bytes_left);

			((_Obj*)_S_start_free)->_M_free_list_link = *__my_free_list;
			*__my_free_list = (_Obj*)_S_start_free;
		}
		_S_start_free = (char*)malloc(alloc_bytes);  // ���� heap �ռ䣬���������ڴ��
		
		if (0 == _S_start_free) {  // heap �ռ䲻�㣬malloc() ʧ��
			size_t __i;
			_Obj* * __my_free_list;
			_Obj* __p;

			for (__i = __size;__i <= (size_t)_MAX_BYTES;__i += (size_t)_ALIGN) 
			{
				__my_free_list = _S_free_list + _S_freelist_index(__i);
				__p = *__my_free_list;
				if (0 != __p) {

					*__my_free_list = __p->_M_free_list_link;
					_S_start_free = (char*)__p;
					_S_end_free = _S_start_free + __i;

				}
			}
			_S_end_free = 0;	
			_S_start_free = (char*)malloc(alloc_bytes);  // ���õ�һ��������

		}


		_S_heap_size += alloc_bytes;
		_S_end_free = _S_start_free + alloc_bytes;

		return(_S_chunk_alloc(__size, __nobjs));  // �ݹ�����Լ�
	}
}


void*_S_refill(size_t __n)
{
 	int __nobjs = 20;
	// ���� _S_chunk_alloc()��ȱʡȡ 20 ��������Ϊ free list ���½ڵ�
	char* __chunk = _S_chunk_alloc(__n, __nobjs);




	//��ȡ����Ŀ�֮�󣬽���Ӧ�� �Ž���Ӧ�±�������  ��������
	_Obj* * __my_free_list;
	_Obj* __result;
	_Obj* __current_obj;
	_Obj* __next_obj;
	int __i;

	// ���ֻ���һ�����ݿ飬��ô������ݿ��ֱ�ӷָ������ߣ����������в��������½ڵ�
	if (1 == __nobjs) 
		return(__chunk);

	__my_free_list = _S_free_list + _S_freelist_index(__n);  // ��������������ݿ�Ĵ�С�ҵ���Ӧ��������  

	__result = (_Obj*)__chunk;
	*__my_free_list = __next_obj = (_Obj*)(__chunk + __n);  // ��0�����ݿ�������ߣ���ַ���ʼ�chunk~chunk + n - 1 

	for (__i = 1; ; __i++) 
	{
		__current_obj = __next_obj;

		__next_obj = (_Obj*)((char*)__next_obj + __n);

		if (__nobjs - 1 == __i) {
			__current_obj->_M_free_list_link = 0;
			break;
		}
		else {
			__current_obj->_M_free_list_link = __next_obj;
		}
	}
	return(__result);
}


static void* allocate(size_t __n)
{
	void* __ret = 0;

	 {
		// ��������ռ�Ĵ�СѰ����Ӧ�Ŀ�������16�����������е�һ����
		_Obj* * __my_free_list= _S_free_list + _S_freelist_index(__n);

		_Obj*  __result = *__my_free_list;

		// ��������û�п������ݿ飬�ͽ������С�ȵ����� 8 �����߽磬Ȼ����� _S_refill() �������
		if (__result == 0)
			__ret = _S_refill(_S_round_up(__n));
		else {
			// ��������������п������ݿ飬��ȡ��һ�������ѿ��������ָ��ָ����һ�����ݿ�  
			*__my_free_list = __result->_M_free_list_link;
			__ret = __result;
		}
	}

	return __ret;
};
//================= ������malloc ʹ�õڶ��������ڴ�����;ʡȥ�˵�һ��==========
//================= ������dellocate�黹�ڴ����̣�ʡȥ�˵�һ��=================
static void deallocate(void* __p, size_t __n)
{

		_Obj* *  __my_free_list= _S_free_list + _S_freelist_index(__n);
		_Obj* __q = (_Obj*)__p;


		__q->_M_free_list_link = *__my_free_list;
		*__my_free_list = __q;

}
//=============����������
int main(int argc, char** argv) {

	
	_S_start_free = 0;
	_S_start_free = 0;
	_S_heap_size = 0;
	_S_free_list[_NFREELISTS] = {0};


	int size_1 = 20;

	void* p1 = allocate(7);

	void* p2 = allocate(17);

	void* p3 = allocate(47);

	void* p4 = allocate(47);

	void* p5 = allocate(6);

	deallocate(p1,7);
	deallocate(p3, 47);
	deallocate(p4, 47);


	return 0;
}
