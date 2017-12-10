--[[

	Generates a config file for Clang-based autocomplete plugins.

	TODO: Make a standalone premake5 module and release it.

--]]

--[[

Based on https://github.com/aerys/minko/blob/9171805751fb3a50c6fcab0b78892cdd4253ee11/module/clang/clang_complete.lua

Minko

Copyright (c) 2014 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

--]]

local clang_complete = {}
clang_complete.generate = function(prj)
	local clang_complete = prj.basedir .. "/.clang_complete"

	os.remove(filename)

	local marked = {}
	local f = io.open(clang_complete, "w")

	if os.is64bit() then
		f:write("-m64\n")
	end

	for cfg in premake.project.eachconfig(prj) do
		if table.contains(cfg.terms, "release") then
			for _, includedir in ipairs(cfg.includedirs) do
				if not marked[includedir] then
					f:write("-I" .. includedir .. "\n")
					marked[includedir] = includedir
				end
			end

			for _, includedir in ipairs(cfg.sysincludedirs) do
				if not marked[includedir] then
					f:write("-I" .. includedir .. "\n")
					marked[includedir] = includedir
				end
			end

			for _, buildoption in ipairs(cfg.buildoptions) do
				if not marked[buildoption] then
					f:write(buildoption .. "\n")
					marked[buildoption] = buildoption
				end
			end

			for _, buildoption in ipairs(premake.tools.gcc.getcxxflags(cfg)) do
				if not marked[buildoption] then
					f:write(buildoption .. "\n")
					marked[buildoption] = buildoption
				end
			end

			for _, define in ipairs(cfg.defines) do
				if not marked[define] then
					f:write("-D" .. define .. "\n")
					marked[define] = define
				end
			end
		end
	end

	f:close()
end

newaction
{
	trigger     = "clang-complete",
	shortname   = "Clang Complete",
	description = "Generate a config file for Clang autocomplete.",

	valid_kinds     = { "ConsoleApp", "WindowedApp", "StaticLib", "SharedLib", "Utility", "Makefile" },
	valid_languages = { "C", "C++" },

	valid_tools =
	{
		cc = { "clang", "gcc" }
	},

	oncleanproject = function(prj)
		os.remove(prj.basedir .. "/.clang_complete")
	end,

	onproject = function(prj)
		print("Generating .clang_complete for project " .. prj.name)
		clang_complete.generate(prj)
	end
}
