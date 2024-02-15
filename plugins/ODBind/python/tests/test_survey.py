import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.geom2d import Geom2D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))

data = {
        'line': 'pytest',
        'trc': np.array([1,2,3],dtype=np.int32),
        'ref': np.array([100.0,200.0,300.0],dtype=np.float32),
        'x': np.array([0.0,100.0,200.0],dtype=np.float64),
        'y': np.array([0.0,0.0,0.0],dtype=np.float64)
        }

@pytest.fixture
def make_geometry(survey):
    assert survey.has2d == True

    with Geom2D.create(survey,'pytest', True) as test:
        test.putdata(data)

    yield survey
    Geom2D.delete(survey,['pytest'])

def test_Survey_class():
    assert 'F3_Demo_2020' in Survey.names()

    try:
        bogus = Survey("bogus")
        assert False
    except:
        assert True
        
    f3demo = Survey("F3_Demo_2020")
    info =  {
                'name': "F3 Demo 2020",
                'type': "2D3D",
                'crs': "EPSG:23031",
                'zdomain': 'twt',
                'xyunit': 'm',
                'zunit': 'ms',
                'srd': 0
            }
    assert f3demo.info() == info
    assert f3demo.bin(610693.97, 6078694.00) == (300, 500)
    assert f3demo.bincoords(610693.97, 6078694.00) == pytest.approx((300.0, 500.0), rel=0.1)
    assert f3demo.coords(300, 500) == pytest.approx((610693.97, 6078694.00), rel=0.01)
    feature =   {
                    'type': 'FeatureCollection',
                    'features': [{
                                    'type': 'Feature',
                                    'properties': info,
                                    'geometry': {'type': 'Polygon',
                                    'coordinates': [[['4.644803', '54.796120'], ['4.643676', '54.942126'],
                                                    ['5.014355', '54.942512'], ['5.014145', '54.796514'],
                                                    ['4.644803', '54.796120']]]}
                                }]
                }
    assert json.loads(f3demo.feature()) == feature
    assert f3demo.has2d == True
    assert f3demo.has3d == True
    assert f3demo.zrange == pytest.approx([0, 1848, 4])

def test_Object_Interface(make_geometry):
    assert 'pytest' in make_geometry.get_object_names('Geometry')
    assert make_geometry.has_object('pytest','Geometry') == True
    assert make_geometry.has_object('notpresent','Geometry') == False
    result = make_geometry.get_object_info('pytest','Geometry')
    assert result['TranslatorGroup'] == 'Geometry'
    assert result == make_geometry.find_object_info('pytest')
    assert result == make_geometry.find_object_info(result['ID'])
    assert result in make_geometry.get_object_infos('Geometry')
    assert make_geometry.get_object_info('notpresent', 'Geometry') == {}