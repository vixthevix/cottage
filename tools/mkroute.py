'''
This file will produce a helper script, for making route functions
You pass in the name of the route, and it will generate a route table for you,
under the routes folder
'''

import sys
import os

# Format of route file.
# We use the literal '%1$s' here so we can simply replace it later, 
# which avoids having to escape all the curly braces { } in the template.
FORMAT_STR = (
    "#include \"../cottage/cottage.h\"\n"
    "\n"
    "NewRouteFunction(%1$sGet) {\n"
    "   return false;\n"
    "}\n"
    "NewRouteFunction(%1$sPost) {\n"
    "   return false;\n"
    "}\n"
    "NewRouteFunction(%1$sPut) {\n"
    "   return false;\n"
    "}\n"
    "NewRouteFunction(%1$sDelete) {\n"
    "   return false;\n"
    "}\n"
    "\n"
    "/*\n"
    "Enter the following into your main code, under where you setup your routes:\n"
    "RouteEntry %1$s = {\n"
    "    .routeGet = %1$sGet,\n"
    "    .routePost = %1$sPost,\n"
    "    .routePut = %1$sPut,\n"
    "    .routeDelete = %1$sDelete\n"
    "};\n"
    "\n"
    "\n"
    "newRoute(YOUR SITE PATH HERE, %1$s);\n"
    "*/\n"
)

def main():
    if len(sys.argv) <= 1:
        print("Please enter the name of the route. Thank you")
        sys.exit(1)
        
    name = sys.argv[1]
    
    # Write out the contents of the file
    contents = FORMAT_STR.replace("%1$s", name)
    
    print(contents)
    
    # make the routes folder
    try:
        os.mkdir("routes", 0o777)
    except OSError:
        print("Could not create routes folder (may already exist).")
        
    # print this into a file
    filename = f"routes/{name}_routes.h"
    try:
        with open(filename, "w") as file:
            file.write(contents)
    except IOError:
        print(f"{filename} does not exist")

if __name__ == "__main__":
    main()