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

def clear_pipeline():
    sources = GetSources()

    for key, src in list(sources.items()):
        Delete(src)

def group_delaunay():
    sources = GetSources()
    del_ident = "delaunay2D"
    del_list = []

    for key, src in list(sources.items()):
        if key[0].startswith(del_ident):
        	del_list.append(src)
        
    groupDatasets1 = GroupDatasets(Input=del_list)	


base = Path.cwd()
print(base)
print(str(base))
name = input("name for folder with pvts and vts")
folder = base / name
print(folder)
name2 = input("name for folder to export csvs")
output = base / name2
print(output)

sample = "cppscript"
output2 = base / sample


pvts_files = sorted(folder.glob("*.pvts"))
vts_files  = sorted(folder.glob("*.vts"))

# Build a set of base stems from pvts
pvts_stems = [f.stem[-2:] for f in pvts_files]
pvts_stem1 = [f.stem for f in pvts_files]

# Filter vts_files: keep only those whose stem does NOT start with any pvts stem
clean_vts = []
for v in vts_files:
    if not any(v.stem.endswith(stem) for stem in pvts_stems):
        clean_vts.append(v)

print("PVTS:", [f.name for f in pvts_files])
print("Filtered VTS:", [f.name for f in clean_vts])

vts_stem1 = [f.stem for f in clean_vts]

# Make readers (unchanged)
pvts_readers = [XMLPartitionedStructuredGridReader(FileName=str(f)) for f in pvts_files]
vts_readers  = [XMLStructuredGridReader(FileName=str(f)) for f in clean_vts]
renderView1 = GetActiveViewOrCreate('RenderView')

print(pvts_readers)
print(vts_readers)
spreadSheetView1 = CreateView('SpreadSheetView')
plength = len(pvts_files)
vlength = len(clean_vts)
i = 0
j = 0

while i < plength:
	cellDatatoPointDataPVTS = CellDatatoPointData(Input=pvts_readers[i])
	cellDatatoPointDataPVTS.CellDataArraytoprocess = ['rho', 'p', 'T', 'H', 'M', 'v']
	cellDatatoPointDataSheet = Show(cellDatatoPointDataPVTS, spreadSheetView1)
	ExportView(str(output) +'/' + pvts_stem1[i] + '.csv', view=spreadSheetView1)
	i+= 1



while j < vlength:
	cellDatatoPointDataVTS = CellDatatoPointData(Input=vts_readers[j])
	cellDatatoPointDataVTS.CellDataArraytoprocess = ['H', 'M', 'T', 'p', 'rho', 'v']
	cellDatatoPointDataSheet = Show(cellDatatoPointDataVTS, spreadSheetView1)
	ExportView(str(output) +'/' + vts_stem1[j] + '.csv', view=spreadSheetView1)
	j+= 1

os.chdir(output)
os.system('make')
os.system('./bulktest')
os.chdir('..')

clear_pipeline()
paraview.simple._DisableFirstRenderCameraReset()


csv_files = sorted(output.glob("*.csv"))
key_ident = "output"
csv_stems = [cs.stem for cs in csv_files]

sorted_csv_files = []
for c in csv_files:
	if c.stem.endswith(key_ident):
		sorted_csv_files.append(c)


csv_readers = [CSVReader(FileName=str(s)) for s in sorted_csv_files]
spreadSheetView1 = CreateView('SpreadSheetView')
renderView1 = FindViewOrCreate('RenderView1', viewtype='RenderView')

delaunay_filters = [] 
for read in csv_readers:
    Show(read, spreadSheetView1)
    Hide(read, spreadSheetView1)
    spreadSheetView1.Update()

    tableToPoints = TableToPoints(Input=read)
    tableToPoints.XColumn = 'Points_0'
    tableToPoints.YColumn = 'Points_1'
    tableToPoints.ZColumn = 'Points_2'
    tableToPointsDisplay = Show(tableToPoints, renderView1)
    Hide(tableToPoints, renderView1)


    delaunay2D = Delaunay2D(Input=tableToPoints)
    delaunay_filters.append(delaunay2D)  
    Show(delaunay2D, renderView1)
    Hide(delaunay2D, renderView1)


# Now group them directly
groupDatasets1 = GroupDatasets(Input=delaunay_filters)
groupDisplay = Show(groupDatasets1, renderView1)
renderView1.Update()









