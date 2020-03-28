#include "sprache_version.hpp"

namespace U
{

namespace
{

const std::string g_sprache_version= SPRACHE_VERSION;

const std::string g_git_revision=
	#include "git_revision.hpp"
	;

const std::string g_full_version= g_sprache_version + ":" + g_git_revision;

} // namespace

const std::string& getSpracheVersion()
{
	return g_sprache_version;
}

const std::string& getGitRevision()
{
	return g_git_revision;
}

const std::string& getFullVersion()
{
	return g_full_version;
}

} // namespace U
