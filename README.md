# P5-Satellite

### Installing sns-3 properly [07-11-2024]

Download ns3.40
```
$ wget https://www.nsnam.org/releases/ns-3-40/documentation
```

cd into contrib/ directory
```
$ cd ns-allinone-3.40/ns-3.40/contrib/
```

Add sns3 and checkout on the working commit.
```
$ git clone https://github.com/sns3/sns3-satellite.git satellite
$ cd satellite
$ git checkout 9dc8f3522a690ea1358dd2ff784ccde567b0de33
```

Add traffic and checkout on the working commit.
```
$ git clone https://github.com/sns3/traffic.git traffic
$ cd traffic
$ git checkout b2ae5681968151e09f2fe5eb584d8eceed7215aa
```

Add magister-stats and checkout on the working commit.
```
$ git clone https://github.com/sns3/stats.git magister-stats
$ cd magister-stats
$ git checkout 5b3cc38f24d823be89acb945ac1cf5f55cbac7b2
```

Configure and build ns3.40
```
$ cd ..
$ ./ns3 configure $NS3_P5
$ ./ns3 build
```
After the build has finished run the tests
```
$ ./test.py
```


