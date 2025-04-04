---
jupyter:
  jupytext:
    formats: ipynb,md
    text_representation:
      extension: .md
      format_name: markdown
      format_version: '1.3'
      jupytext_version: 1.14.5
  kernelspec:
    display_name: Python 3 (ipykernel)
    language: python
    name: python3
---

# Seismic2D Class - ODBind Python Bindings


**If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.**

```python
#import os
#import sys
#odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/odmain/bin/python'
#sys.path.insert(0,odpy_path)
```

```python
import numpy as np
from matplotlib import pyplot as plt
import xarray as xr
from odbind.survey import Survey
from odbind.seismic2d import Seismic2D
from odbind.geom2d import Geom2D
```

## Seismic2D class

The Seismic2D class provides access to 2D seismic datasets in an OpendTect project/survey. A Survey object is required for context.

The user must create a **Seismic2D** object to access a specific volume. There are 2 constructors provided:
-   **Seismic2D( survey:odSurvey, dataset_name:str )** which opens an existing 2D dataset called "dataset_name" if it exists or raises an exception.


```python
f3demo = Survey('F3_Demo_2020')
```

### Seismic2D.info() function
Returns basic information for a 2D seismic dataset in a Python dictionary

```python
data = Seismic2D(f3demo, 'Seis')
data.info()
```

### Seismic2D.feature function
Return a GeoJSON feature collection with the simplified line locations of the 2D seismic dataset. This can be used in map displays.

```python
feature = data.feature()
feature
```

```python
import folium
datamap = folium.Map(location=[52.3,8.0], tiles="openstreetmap", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(feature, popup=folium.GeoJsonPopup(fields=['name'])).add_to(datamap)
datamap
```

### Seismic2D.comp_names property

Return a list of the component names in the 2D seismic dataset.

```python
Seismic2D(f3demo,'Steering_2D').comp_names
```

### Seismic2D.line_names property
Return a list of the seismic lines in the 2D seismic dataset.

```python
data.line_names
```

### Seismic2D.line_info functions
This returns a Pandas DataFrame or a list of dictionaries (depending on the value of Seismic2D.use_dataframe) with basic information for the listed lines (or all lines if no list provided) in this 2D dataset.

```python
data.line_info()
```

### Reading 2D Seismic Data - Seismic2D.getdata

This returns a Xarray Dataset or a tuple (depending on the value of Seismic2D.use_xarray) for the given line in the 2D dataset.

```python
xrline = data.getdata('SSIS-Grid-Strike4')
xrline
```

```python
xr.plot.imshow(xrline['Component 1'], x='trc', y='twt', yincrease=False, cmap='Greys')
```

### Creating 2D Seismic Data - Seismic2D.create and Seismic2D.putdata

Seismic2D.create is used to create a new (or overwrite and existing) 2D seismic dataset.
Seismic2D.putdata is used to write store data for a line into OpendTect

```python
with Seismic2D.create(f3demo, 'pytest', ['comp1'], 'CBVS', True, True) as newdataset:
    newdataset.putdata('test2', xrline, True, True)

newdata = Seismic2D(f3demo,'pytest')
newdata.line_info()
```

### Deleting 2D Seismic Data

```python
newdata.delete_lines(['test2'])
newdata.line_info()
```

```python
Seismic2D.delete(f3demo,['pytest'])
Seismic2D.names(f3demo)
```

```python
Geom2D.delete(f3demo, ['test2'])
Geom2D.names(f3demo)
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 2D seismic datasets in the user provided survey.
### Seismic2D.names() function
Return a list of the 2D seismic datasets in the given survey.

```python
datasets = Seismic2D.names(f3demo)
datasets
```

### Seismic2D.infos() function
These return a dictionary or a Pandas DataFrame (depending on the value of Seismic2D.use_dataframe) with basic information for the listed 2D seismic datasets (or all datasets if no list provided) in the given survey.

```python
Seismic2D.infos(f3demo)
```

### Seismic2D.names_for_line
Returna list of the Seismic2D datasets in the given survey containing the given line

```python
Seismic2D.names_for_line(f3demo, 'SSIS-Grid-Strike1')
```

### Seismic2D.features() function

Returns a GeoJSON feature collection for the listed 2D seismic datasets (or all datasets if no list provided) in the given survey. This can be used to create map displays.

```python
features = Seismic2D.features(f3demo,['Seis'])
features
```

```python
import folium
datamap = folium.Map(location=[52.3,8.0], tiles="openstreetmap", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(features, popup=folium.GeoJsonPopup(fields=['name'])).add_to(datamap)
datamap
```
