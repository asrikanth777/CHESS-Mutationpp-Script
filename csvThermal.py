#tips
#paraview gets confused if you run a script from home, so
#open the folder you want to work in with your exec,input,mesh,output files
#then open folder in terminal and type paraview
#from there you can select your script and have it work from there
#the print statements are to double check you are in the path you want to work from

from pathlib import Path
from paraview.simple import *
import re
import os

paraview.simple._DisableFirstRenderCameraReset()

# this function clears pipeline so it doesnt over clutter after running cpp script
def clear_pipeline():
    sources = GetSources()

    for key, src in list(sources.items()):
        Delete(src)

# this groups all the delaunay filters and groups them for better viewability
def group_delaunay():
    sources = GetSources()
    del_ident = "delaunay2D"
    del_list = []

    for key, src in list(sources.items()):
        if key[0].startswith(del_ident):
        	del_list.append(src)
        
    groupDatasets1 = GroupDatasets(Input=del_list)	


# this part gets the path that PARAVIEW is in
# keep note, if you dont open paraview in terminal from
# a folder location that has everything u need
# it wont work as expected
base = Path.cwd()
print(base)
print(str(base))
name = input("name for folder with pvts and vts")
folder = base / name
print(folder)
name2 = input("name for folder to export csvs")
output = base / name2
print(output)


# collects all pvts and vts files 
pvts_files = sorted(folder.glob("*.pvts"))
vts_files  = sorted(folder.glob("*.vts"))

# Build a set of base stems from pvts
# this is for checking what its importing in later and for writing outputs for csv later
pvts_stems = [f.stem[-2:] for f in pvts_files]
pvts_stem1 = [f.stem for f in pvts_files]

# Filter vts_files: keep only those whose stem does NOT start with any pvts stem
clean_vts = []
for v in vts_files:
    if not any(v.stem.endswith(stem) for stem in pvts_stems):
        clean_vts.append(v)

# read this to make sure you are importing the right things
# if the above code is not there, it will import the pvts and the respective vts files, which arent needed
print("PVTS:", [f.name for f in pvts_files])
print("Filtered VTS:", [f.name for f in clean_vts])

# tracks name to use for outputting csvs
vts_stem1 = [f.stem for f in clean_vts]

# Make readers and import in pvts/vts files
pvts_readers = [XMLPartitionedStructuredGridReader(FileName=str(f)) for f in pvts_files]
vts_readers  = [XMLStructuredGridReader(FileName=str(f)) for f in clean_vts]

# renders them to use actions on them later
renderView1 = GetActiveViewOrCreate('RenderView')

# opens spreadsheet to get cell data
spreadSheetView1 = CreateView('SpreadSheetView')

# iterates through file count to do celldatatopointdata
plength = len(pvts_files)
vlength = len(clean_vts)
i = 0
j = 0

# this is csv conversion, check what values it is carrying out, DONT remove T, P, or v
while i < plength:
	cellDatatoPointDataPVTS = CellDatatoPointData(Input=pvts_readers[i])
	cellDatatoPointDataPVTS.CellDataArraytoprocess = ['rho', 'p', 'T', 'H', 'M', 'v']
	cellDatatoPointDataSheet = Show(cellDatatoPointDataPVTS, spreadSheetView1)
    # shows them so they are properly read to be exported out
    # skipping this step will only give coordinate info, not the values in the cells
	ExportView(str(output) +'/' + pvts_stem1[i] + '.csv', view=spreadSheetView1)
	i+= 1



while j < vlength:
	cellDatatoPointDataVTS = CellDatatoPointData(Input=vts_readers[j])
	cellDatatoPointDataVTS.CellDataArraytoprocess = ['H', 'M', 'T', 'p', 'rho', 'v']
	cellDatatoPointDataSheet = Show(cellDatatoPointDataVTS, spreadSheetView1)
    # shows them so they are properly read to be exported out
    # skipping this step will only give coordinate info, not the values in the cells
	ExportView(str(output) +'/' + vts_stem1[j] + '.csv', view=spreadSheetView1)
	j+= 1

# this goes into output specified where the cppscript should be
os.chdir(output)
os.system('make')
os.system('./thermal')
os.chdir('..')

# clears current pipeline in prep of incoming imports
clear_pipeline()
paraview.simple._DisableFirstRenderCameraReset()


# grabs csvs with only the word output
csv_files = sorted(output.glob("*.csv"))
key_ident = "output"
csv_stems = [cs.stem for cs in csv_files]

sorted_csv_files = []
for c in csv_files:
	if c.stem.endswith(key_ident):
		sorted_csv_files.append(c)

# reads spreadsheet view and "opens" it
csv_readers = [CSVReader(FileName=str(s)) for s in sorted_csv_files]
spreadSheetView1 = CreateView('SpreadSheetView')
renderView1 = FindViewOrCreate('RenderView1', viewtype='RenderView')

# list to collect all delaunay filters made
delaunay_filters = [] 
for read in csv_readers:
    # this makes sure spreadsheet view is open to read csvs
    Show(read, spreadSheetView1)
    Hide(read, spreadSheetView1)
    spreadSheetView1.Update()

    # makes tabletopoints and sets coordinates respectively
    tableToPoints = TableToPoints(Input=read)
    tableToPoints.XColumn = 'Points_0'
    tableToPoints.YColumn = 'Points_1'
    tableToPoints.ZColumn = 'Points_2'
    tableToPointsDisplay = Show(tableToPoints, renderView1)
    Hide(tableToPoints, renderView1)

    # makes delaunay filter to make a nice surface out of the points
    delaunay2D = Delaunay2D(Input=tableToPoints)
    delaunay_filters.append(delaunay2D)  
    Show(delaunay2D, renderView1)
    Hide(delaunay2D, renderView1)


# groups delaunay filters together so u can view the whole field
groupDatasets1 = GroupDatasets(Input=delaunay_filters)
groupDisplay = Show(groupDatasets1, renderView1)
renderView1.Update()









