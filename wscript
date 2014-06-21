# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('dash', ['core', 'applications','point-to-point','wifi','netanim'])
    module.source = [
         'model/dash-client.cc',
         'model/http-parser.cc',
         'model/mpeg-player.cc',
         'model/dash-server.cc',
         'model/http-header.cc',
         'model/mpeg-header.cc',
         'model/algorithms/osmp-client.cc',
         'model/algorithms/svaa-client.cc',
         'model/algorithms/aaash-client.cc',
         'model/algorithms/fdash-client.cc',
         'model/algorithms/raahs-client.cc',
         'model/algorithms/sftm-client.cc',
         'helper/dash-client-helper.cc',
         'helper/dash-server-helper.cc',
#        'model/dash.cc',
#        'helper/dash-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('dash')
    module_test.source = [
        'test/dash-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'dash'
    headers.source = [
         'model/dash-client.h',
         'model/http-parser.h',
         'model/mpeg-player.h',
         'model/dash-server.h',
         'model/http-header.h',
         'model/mpeg-header.h',
         'model/algorithms/osmp-client.h',
         'model/algorithms/svaa-client.h',
         'model/algorithms/aaash-client.h',
         'model/algorithms/fdash-client.h',
         'model/algorithms/raahs-client.h',
         'model/algorithms/sftm-client.h',
         'helper/dash-client-helper.h',
         'helper/dash-server-helper.h',
#        'model/dash.h',
#        'helper/dash-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

