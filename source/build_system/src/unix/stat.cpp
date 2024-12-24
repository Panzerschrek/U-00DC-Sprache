#include <cstdint>
#include <sys/stat.h>


extern "C" uint64_t BKGetFileModificationTimeImpl( const char* const file_name_null_terminated )
{
	struct stat s{};
	const auto res= ::lstat( file_name_null_terminated, &s );
	if( res != 0 )
		return 0;

	// Return nanoseconds.
	return uint64_t( s.st_mtim.tv_sec ) * 1000000000ull + uint64_t( s.st_mtim.tv_nsec );
}

extern "C" int64_t BKGetFileSize( const int32_t fd )
{
	struct stat s{};
	const auto res= ::fstat( fd, &s );
	if( res != 0 )
		return -1;

	return s.st_size;
}

extern "C" bool BKIsDirectory(  const char* const file_name_null_terminated )
{
	struct stat s{};
	const auto res= ::lstat( file_name_null_terminated, &s );
	if( res != 0 )
		return false;

	return (s.st_mode & S_IFMT) == S_IFDIR;
}
