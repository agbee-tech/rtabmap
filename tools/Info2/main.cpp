/*
Copyright (c) 2010-2016, Mathieu Labbe - IntRoLab - Universite de Sherbrooke
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Universite de Sherbrooke nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <rtabmap/core/DBDriver.h>
#include <rtabmap/core/VisualWord.h>
#include <rtabmap/core/Graph.h>
#include <rtabmap/utilite/UDirectory.h>
#include "rtabmap/utilite/UFile.h"
#include "rtabmap/utilite/UStl.h"
#include "rtabmap/core/OccupancyGrid.h"
#include "rtabmap/core/util3d.h"
#include "rtabmap/core/util3d_transforms.h"
#include "rtabmap/core/util3d_filtering.h"
#include "rtabmap/core/util3d_surface.h"
#include "rtabmap/core/util3d_registration.h"
#include "rtabmap/core/util3d_mapping.h"
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/highgui/highgui.hpp>

using namespace rtabmap;

void showUsage()
{
	printf("\nUsage:\n"
			"rtabmap-info [options] \"map.db\"\n"
			"  Options:\n"
			"     --export  \"map.db\"   Export 2D map.\n"
			"     --import  \"map.pgm map.db\"   Import 2D map.\n"
			"\n");
	exit(1);
}

int main(int argc, char * argv[])
{
	if(argc < 2)
	{
		printf("argc = %d", argc);
		showUsage();
	}

	bool export_map = false;
	bool import_map = false;
	std::string importMapPath;
	for(int i=1; i<argc-1; ++i)
	{
		if(strcmp(argv[i], "--export") == 0)
		{
			export_map = true;
		}
		if(strcmp(argv[i], "--import") == 0)
		{
			++i;
			if(i<argc-1)
			{
				importMapPath = uReplaceChar(argv[i], '~', UDirectory::homeDir());
				printf("Import  \"%s\"...\n", importMapPath.c_str());
			}
			import_map = true;
		}
	}

	std::string databasePath = uReplaceChar(argv[argc-1], '~', UDirectory::homeDir());
	if(!UFile::exists(databasePath))
	{
		printf("Database \"%s\" doesn't exist!\n", databasePath.c_str());
		return -1;
	}

	DBDriver * driver = DBDriver::create();
	if(!driver->openConnection(databasePath))
	{
		printf("Cannot open database \"%s\".\n", databasePath.c_str());
		delete driver;
		return -1;
	}

	if(export_map){
		if(!driver)
		{
			printf("Cannot export 2D map. A database must must loaded first...");
			return -1;
		}
		float xMin, yMin, cellSize;
		cv::Mat map = driver->load2DMap(xMin, yMin, cellSize);
		if(map.empty())
		{
			printf("Cannot export 2D map. A database must must loaded first...");
			return -1;
		}
		else
		{
			cv::Mat map8U = rtabmap::util3d::convertMap2Image8U(map, true);
			std::string name = "rtabmap.pgm";
			std::string path = uReplaceChar(name, '~', UDirectory::homeDir());
			cv::imwrite(path, map8U);
			printf("Exported %s!.\n", path.c_str());
		}
	}

	if(import_map){
		if(!driver)
		{
			printf("Cannot import 2D map. A database must must loaded first...");
			return -1;
		}
		float xMin, yMin, cellSize;
		cv::Mat mapOrg = driver->load2DMap(xMin, yMin, cellSize);
		if(mapOrg.empty())
		{
			printf("Cannot import 2D map. The database doesn't contain a saved 2D map.");
		}
		else
		{
			if(!importMapPath.empty())
			{
				cv::Mat map8U = cv::imread(importMapPath, cv::IMREAD_UNCHANGED);
				cv::Mat map = rtabmap::util3d::convertImage8U2Map(map8U, true);

				if(mapOrg.cols == map.cols && mapOrg.rows == map8U.rows)
				{
					driver->save2DMap(map, xMin, yMin, cellSize);
					printf("Imported %s!\n", importMapPath.c_str());
				}
				else
				{
					printf("Cannot import %s as its size doesn't match the current saved map. Import 2D Map action should only be used to modify the map saved in the database.", importMapPath.c_str());
				}
			}
		}

	}

	return 0;
}
