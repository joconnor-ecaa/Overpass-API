#!/usr/bin/env bash

# Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
#
# This file is part of Overpass_API.
#
# Overpass_API is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Overpass_API is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.

if [[ -z $1  ]]; then
{
  echo "Usage: $0 test_size"
  echo
  echo "An appropriate value for a fast test is 40, a comprehensive value is 2000."
  exit 0
};
fi

BASEDIR="`pwd`/../"
INPUTDIR="../../input/vlt_model/"

mkdir -p run/vlt_model
rm -fR run/vlt_model/*

pushd run/vlt_model

date '+%T'; $BASEDIR/bin/update_database --db-dir=./ --keep-attic <$INPUTDIR/init.osm

date '+%T'
{ cat $INPUTDIR/now.ql; echo "local; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_global.out 2>now_global.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local ll; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_global_ll.out 2>now_global_ll.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local llb; out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_global_llb.out 2>now_global_llb.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local (52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_bbox.out 2>now_bbox.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local ll(52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_bbox_ll.out 2>now_bbox_ll.err
date '+%T'
{ cat $INPUTDIR/now.ql; echo "local llb(52,8,53,9); out geom;"; } \
  | $BASEDIR/bin/osm3s_query --concise --db-dir=./ >now_bbox_llb.out 2>now_bbox_llb.err
date '+%T'

for i in *.err; do
  diff -q "../../expected/vlt_model/$i" "$i"
done

for i in *.out; do
  diff -q "../../expected/vlt_model/$i" "$i"
done

popd
