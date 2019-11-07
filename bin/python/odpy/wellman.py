import numpy as np

import odpy.common as odcommon
import odpy.dbman as oddbman

wellmanexe = 'od_WellMan'
wlldbdirid = '100050'
wlltrlgrp = 'Well'
dgbtrl = 'dGB'

dblist = None

def getNames( reload=False, args=None ):
  if args == None:
    args = odcommon.getODArgs()
  return getWellDBList(reload,args)['Names']

def getInfo( wllnm, reload=False, args=None ):
  dbkey = getDBKey( wllnm, reload=reload, args=args )
  return oddbman.getInfoByNm( dbkey, exenm=wellmanexe,args=args )  

#def getTrack()
#def getLogNames()
#def getLog()

def getDBKey( wllnm, reload=False, args=None ):
  global dblist
  dblist = getWellDBList(reload,args)
  return oddbman.getDBKeyForName( dblist, wllnm )  

def getWellDBList( reload, args=None ):
  global dblist
  if dblist != None and not reload:
    return dblist

  if args == None:
    args = odcommon.getODArgs()
  dblist = oddbman.getDBList(wlltrlgrp,exenm=wellmanexe,args=args)
  return dblist

