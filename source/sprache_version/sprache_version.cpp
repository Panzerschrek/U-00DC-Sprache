#include <string>
#include "sprache_version.hpp"

namespace U
{

namespace
{

const std::string_view g_sprache_version= SPRACHE_VERSION;

const std::string_view g_git_revision=
	#include "git_revision.hpp"
	;

const std::string g_full_version= std::string(g_sprache_version) + ":" + std::string(g_git_revision);

} // namespace

std::string_view getSpracheVersion()
{
	return g_sprache_version;
}

std::string_view getGitRevision()
{
	return g_git_revision;
}

std::string_view getFullVersion()
{
	return g_full_version;
}

} // namespace U
