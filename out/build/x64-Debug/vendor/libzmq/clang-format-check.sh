#!/bin/sh
FAILED=0
IFS=";"
FILES="../../../../../include/Varjo.h;../../../../../include/Varjo_d3d11.h;../../../../../include/Varjo_d3d12.h;../../../../../include/Varjo_datastream.h;../../../../../include/Varjo_events.h;../../../../../include/Varjo_export.h;../../../../../include/Varjo_gl.h;../../../../../include/Varjo_layers.h;../../../../../include/Varjo_math.h;../../../../../include/Varjo_mr.h;../../../../../include/Varjo_types.h;../../../../../include/Varjo_types_d3d11.h;../../../../../include/Varjo_types_datastream.h;../../../../../include/Varjo_types_gl.h;../../../../../include/Varjo_types_layers.h;../../../../../include/Varjo_types_mr.h;../../../../../include/Varjo_types_vk.h;../../../../../include/Varjo_types_world.h;../../../../../include/Varjo_version_defines.h;../../../../../include/Varjo_vk.h;../../../../../include/Varjo_world.h;../../../../../src/main.cpp;../../../../../src/number_atlas_base64.hpp"
IDS=$(echo -en "\n\b")
for FILE in $FILES
do
	clang-format -style=file -output-replacements-xml "$FILE" | grep "<replacement " >/dev/null &&
    {
      echo "$FILE is not correctly formatted"
	  FAILED=1
	}
done
if [ "$FAILED" -eq "1" ] ; then exit 1 ; fi
