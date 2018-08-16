#-----------------------------------------------------------------
# pycparser: use_cpp_libc.py
#
# Shows how to use the provided 'cpp' (on Windows, substitute for
# the 'real' cpp if you're on Linux/Unix) and "fake" libc includes
# to parse a file that includes standard C headers.
#
# Copyright (C) 2008-2011, Eli Bendersky
# License: BSD
#-----------------------------------------------------------------
import sys

# This is not required if you've installed pycparser into
# your site-packages/ with setup.py
#
sys.path.extend(['.', '..'])

# Portable cpp path for Windows and Linux/Unix
CPPPATH = 'cpp.exe' if sys.platform == 'win32' else 'cpp'

import pycparser
import os,shutil
import re
from pycparser import c_ast
from subprocess import Popen, PIPE, STDOUT

class UndefinedType(BaseException): pass
class NameConfflict(BaseException): pass
class UnnamedUnion (BaseException): pass
class UndefinedUnion(BaseException):pass
class UndefinedUnionInNameStructDict(BaseException):pass

#----- from pycparser, tune the calling of Popen by adding stderr -----#

def preprocess_file(filename, cpp_path='cpp', cpp_args=''):
    """ Preprocess a file using cpp.

        filename:
            Name of the file you want to preprocess.

        cpp_path:
        cpp_args:
            Refer to the documentation of parse_file for the meaning of these
            arguments.

        When successful, returns the preprocessed file's contents.
        Errors from cpp will be printed out.
    """
    path_list = [cpp_path]
    if isinstance(cpp_args, list):
        path_list += cpp_args
    elif cpp_args != '':
        path_list += [cpp_args]
    path_list += [filename]

    try:
        # Note the use of universal_newlines to treat all newlines
        # as \n for Python's purpose
        #
        pipe = Popen(   path_list,
                        stdout=PIPE,
                        stderr=PIPE,
                        universal_newlines=True)
        text = pipe.communicate()[0]
    except OSError as e:
        raise RuntimeError("Unable to invoke 'cpp'.  " +
            'Make sure its path was passed correctly\n' +
            ('Original error: %s' % e))

    return text


def parse_file(filename, use_cpp=False, cpp_path='cpp', cpp_args='',
               parser=None):
    """ Parse a C file using pycparser.

        filename:
            Name of the file you want to parse.

        use_cpp:
            Set to True if you want to execute the C pre-processor
            on the file prior to parsing it.

        cpp_path:
            If use_cpp is True, this is the path to 'cpp' on your
            system. If no path is provided, it attempts to just
            execute 'cpp', so it must be in your PATH.

        cpp_args:
            If use_cpp is True, set this to the command line arguments strings
            to cpp. Be careful with quotes - it's best to pass a raw string
            (r'') here. For example:
            r'-I../utils/fake_libc_include'
            If several arguments are required, pass a list of strings.

        parser:
            Optional parser object to be used instead of the default CParser

        When successful, an AST is returned. ParseError can be
        thrown if the file doesn't parse successfully.

        Errors from cpp will be printed out.
    """
    if use_cpp:
        text = preprocess_file(filename, cpp_path, cpp_args)
    else:
        with open(filename, 'rU') as f:
            text = f.read()

    if parser is None:
        parser = pycparser.CParser()
    return parser.parse(text, filename)


def generate_trace_log_file(trace_log_file, all_h_file_list, current_dir):
    '''
    generate the trace_log_file according to all the h files and the c files
    '''
    os.chdir(current_dir)
    flag = 0
    f = open( trace_log_file , "w")
    f.write( "#include \"BaseDef.h\"\n" )
    f.write( "#include \"Proj_Config.h\"\n" )
    f.write( "#include \"HAL_MemoryMap.h\"\n" )

    '''
    one of L0_ATALibAdapter.h and L0_Ahci.h may exist ,and it must before L0_ATAGenericCmdLib.h
    '''
    for hn in all_h_file_list:
        if hn == "L0_ATALibAdapter.h":
            flag = 1
        elif hn == "L0_Ahci.h":
            flag = 2

    
    for hn in all_h_file_list:
        if hn != "BaseDef.h" and hn != "HAL_MemoryMap.h" and hn != "Proj_Config.h":
            if hn.startswith("HAL"):
                f.write( "#include \"" + hn + "\"\n")
    '''
    for hn in all_h_file_list:
        if hn != "BaseDef.h" and hn != "HAL_MemoryMap.h" and hn != "Proj_Config.h":
            if False == hn.startswith("HAL"):
                f.write( "#include \"" + hn + "\"\n")
    '''

    for hn in all_h_file_list:
        if hn != "BaseDef.h" and hn != "HAL_MemoryMap.h" and hn != "Proj_Config.h":
            if False == hn.startswith("HAL"):
                if hn == "L0_ATAGenericCmdLib.h":
                    if flag == 1:
                        f.write( "#include \"L0_ATALibAdapter.h\"\n")
                        flag = 3
                    elif flag == 2:
                        f.write( "#include \"L0_Ahci.h\"\n")
                        flag = 3
                    f.write( "#include \"L0_ATAGenericCmdLib.h\"\n")
                elif hn == "L0_ATALibAdapter.h":
                    if flag != 3:
                        f.write( "#include \"L0_ATALibAdapter.h\"\n")
                        flag = 0
                elif hn == "L0_Ahci.h":
                    if flag != 3:
                        f.write( "#include \"L0_Ahci.h\"\n")
                        flag = 0
                else:
                    f.write( "#include \"" + hn + "\"\n")

    
    f.close()

#------------------------ parse configuration file ---------------------------#

def parse_args_preprocess():
    """
        priority:
         1. input parameters
         2. *.ini
         3. default value in this file.
         4. change work dir (default: the directory where trace_log.c is)
    """
    
    current_dir = sys.argv[ 1 ]
    mcu = sys.argv[ 2 ]
    log_format_generate_path = sys.argv[ 3 ]
    hpath = sys.argv[ 4 ]
    macro = sys.argv[ 5 ]
    source = sys.argv[ 6 ]

    """
        Bug Description:
            When generate log format file of SATA based trace log , our trace log implement
        will analyse the TRACE_LOG function in "firmware\algorithm\Misc\FW_Debug.c", but
        actually most of them won't be compiled, because they are in the code segment 
        controlled by some macro, and we don't define these macro in this compile. So 
        trace log will give errors when meet the structs in FW_Debug.c.
        
        Current Solution:
            Because this bug only happend when generate log format file of SATA based trace log,
        and the definition of the structs trace log don't know is in "firmware\HAL\SGE\HAL_SGE.h",
        so we can append "firmware\HAL\SGE" after hpath.
    """
    #Start apped "firmware\HAL\SGE" after hpath. add by Johnson Zhang 2015/8/21
    if("-DHOST_SATA" in macro):
        current_dir_list = current_dir.split("\\")
        hal_sge_dir = "/".join(current_dir_list[0:-3])+ "/firmware/HAL/SGE"
        hpath += " " + hal_sge_dir
    #End apped "firmware\HAL\SGE" after hpath. add by Johnson Zhang 2015/8/21
    
    #print "current_dir:%s" %current_dir
    temple_dir = os.path.join( current_dir,"firmware" )
    if False == os.path.exists( temple_dir ):
        os.makedirs( temple_dir )
    
    if False == os.path.exists( log_format_generate_path ):
        os.makedirs( log_format_generate_path )

    # fix output configura file for decoder.
    if mcu == 'MCU0':
        output_config_file_name = os.path.join( log_format_generate_path,"log_format_file0.ini" )
    elif mcu == 'MCU1':
        output_config_file_name = os.path.join( log_format_generate_path,"log_format_file1.ini" )
    elif mcu == 'MCU2':
        output_config_file_name = os.path.join( log_format_generate_path,"log_format_file2.ini" )
    else:
        print '[Error] not correct args!'

    hal_traclog_h_destpath = os.path.join( log_format_generate_path,"HAL_TraceLog.h" )

    trace_log_file = os.path.join( current_dir,"firmware\\TraceLogInclude.c" )

    c_fn_enum_name = "C_FILE_NAME_ENUM"
    
    # get source hfile macro
    destfolder = os.path.join(current_dir,"firmware")
    all_h_file_list  = list()
    for token in hpath.split(" "):
        if os.path.isdir( token ):
            for root, dirs, files in os.walk( token ):
                for fn in files:
                    if ( fn[-2:].upper() == ".H" ) and ( fn not in all_h_file_list ):
                        fatherfolder,myname = os.path.split( os.path.join( root,fn ) )
                        if fatherfolder == token:
                            all_h_file_list.append( os.path.basename( fn ) )
                            sourcepath = os.path.join( root,fn )
                            directpath = os.path.join( destfolder,fn )
                            shutil.copyfile( sourcepath ,directpath )
                            if ( fn == "HAL_TraceLog.h" ):
                                shutil.copyfile( sourcepath ,hal_traclog_h_destpath )
    #print "all_h_file_list:%s" %all_h_file_list
    
    generate_trace_log_file(trace_log_file, all_h_file_list, current_dir)
    
    args_for_cpp = list()
    args_for_cpp.append( "-DONLY_FOR_TRACE" )
    args_for_cpp.append( "-DC_PARSER" )
    for token in macro.split(" "):
        args_for_cpp.append(token)
    args_for_cpp.append( "-Ifirmware" )
    #print "args_for_cpp:%s" %args_for_cpp

    all_c_file_list  = list()
    for token in source.split(" "):
        all_c_file_list.append(token)
    #print "all_c_file_list:%s" %all_c_file_list

    # get cpp.exe
    exe_path = os.path.dirname( __file__ )
    cpp_path = os.path.join( exe_path, CPPPATH )
    if False == os.path.exists( cpp_path ):
        print '[Error] cpp path: %s doesn\'t exist!' % cpp_path
        return (False, )
    
    return ( True, trace_log_file, all_c_file_list, all_h_file_list,
             cpp_path, args_for_cpp, c_fn_enum_name, output_config_file_name, temple_dir)

#-----------------------------------------------------------------------------#

# default size for enum type
enum_type_size = 32

# default size for pointer type on 32bit platform
# For 64bit platform, should change it to 64
ptr_type_size  = 32

# the flag whether we find out the target field or not.
# set it to False initially.
g_flag_foundout = False

# the remaining size after the target field has been found out.
g_remaining_size = 0

# As for raw data (without any structure or union), decode and display it
# with byte/word/dword/qword type
common_type_dict = {
    "byte"  : ( "BYTE", 1 ),
    "word"  : ( "WORD", 2 ),
    "dword" : ( "DWORD", 4 ),
    "qword" : ( "QWORD", 8 ),
    "U8"    : ( "UBYTE", 1),
    "S8"    : ( "BYTE", 1),
    "U16"   : ( "UWORD", 2),
    "S16"   : ( "WORD", 2),
    "U32"   : ( "UDWORD", 4),
    "S32"   : ( "DWORD", 4),
}


# initial type<->length dictory.
# With traversing the whole AST, the dictory should be appended.
#
type_length_dict = { 
    "char"              : 8,
    "unsigned char"     : 8,
    "short"             : 16,
    "unsigned short"    : 16,
    "long"              : 32,
    "unsigned long"     : 32,
    "bool"              : 32,
    "BOOL"              : 32,
    "int"               : 32,
    "unsigned int"      : 32,
    "longlong"          : 64,
    "unsigned longlong" : 64,
}

# the type<-->length dict should be read-only.
# It represents the basic data types and the corresponding size.
global_basic_type_length_dict = {
    "char"              : 8,
    "unsigned char"     : 8,
    "short"             : 16,
    "unsigned short"    : 16,
    "bool"              : 32,
    "BOOL"              : 32,
    "long"              : 32,
    "unsigned long"     : 32,
    "int"               : 32,
    "unsigned int"      : 32,
    "longlong"          : 64,
    "unsigned longlong" : 64,
}

def is_structure( node ):
    """ determine whether the node is a type of structure/union/Enum/typedef.
        Only the above types will be processed by this program.
    """
    t = type( node )
    if ( issubclass( t, c_ast.Typedef ) ):
        ret = True
    elif ( issubclass( t, ( c_ast.Decl, c_ast.PtrDecl, c_ast.TypeDecl ) ) ):
        ret = is_structure( node.type )
    elif ( issubclass( t, 
            ( c_ast.IdentifierType,
              c_ast.BinaryOp,
              c_ast.Constant,
              c_ast.FuncDecl,
              c_ast.FuncDef,
              ) ) ):
        ret = False
    elif ( issubclass( t, ( c_ast.Struct, c_ast.Union ) ) ):
        if ( ( node.decls != None ) and ( len( node.decls ) > 0 ) ):
            ret = True
        else:
            ret = False
    elif ( issubclass( t, c_ast.Enum ) ):
        if ( node.name == None ):
            ret = False
        else:
            ret = True
    else:
        ret = False
    return ret

def name_in_tuple( name, *arg ):
    """
        name : represents the target field name.
        *arg : is a tuple. It represents all field names in high level name space so far.

        determine whether the "name" is in "*arg"

        The latter the position of a name space is in "arg", the closer it is to the "name"
        So traverse it REVERSELY to find out about the "name".
    """
    if name == None:
        return ( False, None)
    for x in reversed( arg ):
        if ( True == x.has_key( name ) ):
            return ( True, x)
    return ( False, None)

def build_type_length_dict( node, *arg ):
    """
        calculate the size of the node.
        return value: should be a number.
    """
    t = type( node )
    ret = 0
    if ( issubclass( t, c_ast.Typedef ) ):
        (flag, n ) = name_in_tuple( node.name, *arg )
        if ( True == flag ):
            ret = n[ node.name ]
        else:
            ret = build_type_length_dict( node.type, *arg )
            arg[-1][ node.name ] = ret
    elif ( issubclass( t, c_ast.Decl ) ):
        if ( None != node.bitsize ):
            ret = build_type_length_dict( node.bitsize, *arg )
        else:
            ret = build_type_length_dict( node.type, *arg )
    elif ( issubclass( t, c_ast.IdentifierType ) ):
        if ( len( node.names ) > 1 ):
            type_name = " ".join( node.names )
        else:
            type_name = node.names[0]
        ( flag, n ) = name_in_tuple( type_name, *arg )
        if ( True == flag ):
            ret = n[ type_name ]
        #else:
        #    raise UndefinedType, node

    elif ( issubclass( t, c_ast.Struct ) ):
        (flag, n ) = name_in_tuple( node.name, *arg )
        if ( True == flag ):
            ret = n[ node.name ]
        else:
            local_list = dict()
            # push local list
            arg += ( local_list, )
            if hasattr(node.decls, '__iter__') :
                for child in node.decls:
                    ret += build_type_length_dict( child, *arg )
            # pop local_list
            arg = arg[:-1]
            arg[-1][ node.name ] = ret
    elif ( issubclass( t, ( c_ast.TypeDecl, c_ast.Typename ) ) ):
        ret = build_type_length_dict( node.type, *arg )
    elif ( issubclass( t, c_ast.Constant ) ):
        ret = int( node.value )
    elif ( issubclass( t, c_ast.Union ) ):
        (flag, n ) = name_in_tuple( node.name, *arg )
        if ( True == flag ):
            ret = n[ node.name ]
        else:
            local_list = dict()
            # push local list
            arg += ( local_list, )
            max_size = 0
            for child in node.decls:
                ret = build_type_length_dict( child, *arg )
                #arg[-1][ node.name ] = ret
                if ( ret > max_size ):
                    max_size = ret
            # pop local_list
            arg = arg[:-1]
            arg[-1][ node.name ] = max_size
            ret = max_size
    elif ( issubclass( t, c_ast.Enum ) ):
        ret = enum_type_size
    elif ( issubclass( t, c_ast.PtrDecl ) ):
        ret = ptr_type_size
    elif ( issubclass( t, c_ast.FuncDecl ) ):
        ret = ptr_type_size
    elif ( issubclass( t, c_ast.ArrayDecl ) ):
        dim = build_type_length_dict( node.dim, *arg )
        child_len = build_type_length_dict( node.type, *arg )
        ret = dim * child_len
    elif ( issubclass( t, c_ast.BinaryOp ) ):
        left = build_type_length_dict( node.left, *arg )
        right = build_type_length_dict( node.right, *arg )
        if not((node.op == '/') and (right == 0)):
            ret = eval( "%d %s %d" % (left, node.op, right ) )
    elif ( issubclass( t, c_ast.UnaryOp ) ):
        if ( node.op == "sizeof" ):
            ret = build_type_length_dict( node.expr, *arg ) / 8
        #else:
        #    raise UndefinedType, node
    else:
         pass
    return ret 
    

def build_name_struct_dict( node, *arg ):
    """ 
        build a dict only for struct/unions in top-level namespace.
        arg[ 0 ] : top-level struct/union/typedefs in global name space
    """
    t = type( node )
    ret = 0
    if ( issubclass( t, c_ast.Typedef ) ):
        if ( node.name == None ):
            build_name_struct_dict( node.type, *arg )
        else:
            if ( True == arg[ 0 ].has_key( node.name ) ):
                return
            else:
                arg[ 0 ][ node.name ] = node
                build_name_struct_dict( node.type, *arg )
    elif ( issubclass( t, ( c_ast.Struct, c_ast.Union ) ) ):
        if ( node.name != None ):
            if (  False == arg[ 0 ].has_key( node.name ) ):
                arg[ 0 ][ node.name ] = node
        else:
            return
        return
    elif ( issubclass( t, ( c_ast.TypeDecl, c_ast.Typename, c_ast.Decl ) ) ):
        build_name_struct_dict( node.type, *arg )
    else:
         return

def build_common_type_list( ret_list, total_field_size, struct_name ):
    """
        for common type like "byte", "word", "dword" and "qword", build a list like:
        [ (type, variable, size), (type, variable, size), ... ]

        the length is depended on "total_field_size" and the size of "struct_name"
    """
    global common_type_dict
    assert( common_type_dict.has_key( struct_name ) )
    ( type_name, type_size ) = common_type_dict[ struct_name ]
    for n in range( total_field_size / ( 8 * type_size ) ):
        ret_list.append( ( type_name, "%s[%d]" % ( type_name, n), 8 * type_size ) )

    return ret_list



def collect_field_name( node, target_field_name, flag_collect, field_size,  ret_list, var_path_list, type_list, *arg ):
    """
        1. find out the field specified by target_field_name in the "node".
        2. if yes, continue to traverse the rest fields by g_remaining_size.
             a) append the field name into a list
             b) decrease g_remaining_size by the above field.
             c) If g_remaining_size is equal to zero, do nothing.
           if no, return
        arg[ 0 ] : type_length_dict for top level struct/unoin/typedef.
        arg[ 1 ] : the type<->length pair dict of the second level struct/unoin
        arg[ 2 ] : the type<->length pair dict of the third  level struct/unoin

        return value:
        the size of the "node"

        ret_list should be appended in the following cases:
         1. c_ast.IdentifierType: and the type of the node is basic type in global_basic_type_length_dict
         2. c_ast.FuncDecl      : a pointer to a function
         3. c_ast.PtrDecl       : a pointer
    """
    global g_flag_foundout
    global g_remaining_size
    global global_basic_type_length_dict
    if ( g_remaining_size <= 0 ):
        return 0
    pfn = collect_field_name
    t = type( node )
    ret = 0

    if ( issubclass( t, c_ast.IdentifierType ) ):
        if ( len( node.names ) > 1 ):
            type_name = " ".join( node.names )
        else:
            type_name = node.names[0]
        if ( global_basic_type_length_dict.has_key( type_name ) ):
            #print "IdentifierType %s g_flag_foundout %d flag_collect %d" % ( type_name, g_flag_foundout, flag_collect )
            ret = global_basic_type_length_dict.get( type_name )
            if (  g_flag_foundout  and flag_collect ):
                var_size = ret
                if ( field_size != 0 ):
                    var_size = field_size
                ret_list.append( ( type_name, ".".join( var_path_list ), var_size ) )
                g_remaining_size -= var_size
                #print "remain size %d" % g_remaining_size
                #print "%s %d, remaining g_remaining_size %d" % ( type_name, ret, g_remaining_size )
        else:
            ( flag, n ) = name_in_tuple( type_name, *arg )
            if ( True == flag ):
                ret = pfn( n[ type_name ], target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
            else:
                raise UndefinedType, node
    elif ( issubclass( t, c_ast.FuncDecl ) ):
        ret = ptr_type_size
        if ( g_flag_foundout and flag_collect ):
            type_name = "pfn"
            ret_list.append( ( type_name, ".".join( var_path_list ), ret ) )
            g_remaining_size -= ret
    elif ( issubclass( t, c_ast.PtrDecl ) ):
        ret = ptr_type_size
        if ( g_flag_foundout and flag_collect ):
            type_name = "ptr"
            ret_list.append( ( type_name, ".".join( var_path_list ), ret ) )
            g_remaining_size -= ret
    elif ( issubclass( t, c_ast.Enum ) ):
        ret = enum_type_size
        if ( g_flag_foundout and flag_collect ):
            if ( node.name != None ):
                type_name = "enum %s" % node.name
            else:
                type_name = "enum"
            ret_list.extend( ( type_name, ".".join( var_path_list ), ret ) )
            g_remaining_size -= ret
    elif ( issubclass( t, c_ast.Decl ) ):
        if ( node.name != None ):
            var_path_list.append( node.name )
            type_name = ".".join( var_path_list )
            #print type_name
            if ( g_flag_foundout == False ):
                if ( type_name == target_field_name ):
                    g_flag_foundout = True
            else:
                if ( type_name == target_field_name ):
                    raise "NameConfflict", node
         
        if ( None != node.bitsize ):
            ret = pfn( node.bitsize, target_field_name,   flag_collect, 0,  ret_list, var_path_list, type_list, *arg )
            pfn( node.type, target_field_name,   flag_collect, ret,  ret_list, var_path_list, type_list, *arg )
        else:
            ret = pfn( node.type, target_field_name,   flag_collect, 0,  ret_list, var_path_list, type_list, *arg )
        if ( node.name != None ):
            var_path_list.pop()

    elif ( issubclass( t, c_ast.Struct ) ):
        ret = 0
        if ( node.decls == None ):
            if ( node.name == None ):
                raise "UnnamedStruct", node
            else:
                (flag, n ) = name_in_tuple( node.name, *arg )
                if ( False == flag ):
                    raise "UndefinedStruct", node
                new_node = n.get( node.name )
                if ( new_node == None ):
                    raise "UndefinedStructInNameStructDict", node
                else:
                    ( new_node, node ) = (node, new_node )
        local_list = dict()
        # push local list
        arg += ( local_list, )
        for child in node.decls:
            ret += pfn( child, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
        # pop local_list
        arg = arg[:-1]
        if ( node.name != None ):
            arg[-1][ node.name ] = node
    elif ( issubclass( t, c_ast.Union ) ):
        ret = 0
        if ( node.decls == None ):
            if ( node.name == None ):
                raise UnnamedUnion, node
            else:
                (flag, n ) = name_in_tuple( node.name, *arg )
                if ( False == flag ):
                    raise UndefinedUnion, node

                new_node = n.get( node.name )
                if ( new_node == None ):
                    raise UndefinedUnionInNameStructDict, node
                else:
                    ( new_node, node ) = (node, new_node )

        local_list = dict()
        # push local list
        arg += ( local_list, )
        #print "Union %s" % node.name
        max_size = 0
        max_node = None
        target_field_node = None
        for child in node.decls:
            old_flag_foundout = g_flag_foundout
            # don't collect the first time. Just find out the field with the maximum size.
            ret = pfn( child, target_field_name,   False, field_size,  ret_list, var_path_list, type_list, *arg )
            if ( ret > max_size ):
                max_size = ret
                max_node = child
            if ( g_flag_foundout == True ) and ( old_flag_foundout == False ):
                # find out the target field, select it.
                target_field_node = child
                #print "hit! %s " % child.name

        # pop local_list
        if ( target_field_node != None ):
            g_flag_foundout = False
            n = target_field_node
        else:
            n = max_node
        pfn( n, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
        arg = arg[:-1]
        if ( node.name != None ):
            arg[-1][ node.name ] = max_size
        ret = max_size
    elif ( issubclass( t, ( c_ast.TypeDecl, c_ast.Typename ) ) ):
        ret = pfn( node.type, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
    elif ( issubclass( t, c_ast.Typedef ) ):
        ret = pfn( node.type, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
    elif ( issubclass( t, c_ast.Constant ) ):
        ret = int( node.value )
    elif ( issubclass( t, c_ast.ArrayDecl ) ):
        dim = pfn( node.dim, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
        array_name = var_path_list.pop()
        #print "dim:%s target_field_name:%s " %dim %target_field_name
        for n in range( dim ):
            var_path_list.append( "%s[%d]" % ( array_name, n ) )
            if ( g_flag_foundout == False ):
                if ( var_path_list[ -1 ] == target_field_name ):
                    g_flag_foundout = True
            else:
                if ( var_path_list[ -1 ] == target_field_name ):
                    raise "NameConfflict", node

            child_len = pfn( node.type, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
            var_path_list.pop()
        var_path_list.append( array_name )
        if dim == 0:
            ret = 0
        else:
            ret = dim * child_len
    elif ( issubclass( t, c_ast.BinaryOp ) ):
        left = pfn( node.left, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
        right = pfn( node.right, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg )
        ret = eval( "%d %s %d" % (left, node.op, right ) )
    elif ( issubclass( t, c_ast.UnaryOp ) ):
        if ( node.op == "sizeof" ):
            ret = pfn( node.expr, target_field_name,   flag_collect, field_size,  ret_list, var_path_list, type_list, *arg ) / 8
        else:
            raise UndefinedType, node
    else:
        pass
    return ret 



def decode_trace_log_statement( output_log_format_file, trace_log_statement_list ):
    """
        decode trace log statement list and write it to output_log_format_file

        The format of each item in trace_log_statement_list should be expected to be a tuple
        outputed by collect_trace_log_statement like the following:

        ( file_num, line_num, data_size, struct_name, field_name, log_txt )

        The format in output_log_format_file is just like the following:

        s[ 0 ] , s[ 1 ] , s[ 2 ]    , s[ 3 ]    , s[ 4 ]   , s[ 5 ]
        FileNum, LineNum, Size(bit), StructName, FieldName, log_txt      , Type, VariableName, FieldSize, Type, ... , FieldSize
        12     , 123    , 16        , MY_STRUCT , A.AChild1, "want to log", int , Tag         , ...
    """
    f = open( output_log_format_file, "w" )
    for s in trace_log_statement_list:
        ret_list = build_trace_information( s[ 2], s[ 3 ], s[ 4 ], name_structure_dict )
        f.write( "%3d, %4d, %2d, %10s, %15s, %32s," % s )
        for item in ret_list:
            f.write( "%12s, %12s, %d," % item )
        f.write("!@#$\n")

    f.close()

def parse_trace_log_c_file( file_name, path, args, name_structure_dict, type_length_dict):
    """ parse trace log include file for all structures/unions in header files.
    """
    ast = parse_file(file_name, use_cpp=True,
            cpp_path=path,
            cpp_args=args )
    #ast.show()
    # find out all valid struct/enum/unions
    target_list = list()
    for n in ast.ext:
        if ( is_structure( n ) == True ):
            #n.show()
            target_list.append( n )
            
    for n in target_list:
        #print "parse %s" % n.name
        #n.show()
        ret = build_type_length_dict( n, type_length_dict )
        build_name_struct_dict( n, name_structure_dict )


def build_trace_information( total_field_size, struct_name, field_name, name_structure_dict ):
    """
        total_field_size: 
        1. find out the struct node specified by "struct_name" from the dict "name_structure_dict"
        2. find out the fields specified by "field_name" in the struct specified by "struct_name"
        3. collect fields by the number specified by "total_field_size" and append those into a return list.
        return the above list.
    """

    global g_remaining_size
    global g_flag_foundout

    collect_list = list()
    if ( field_name == struct_name ):
        g_flag_foundout = True
    else:
        g_flag_foundout = False
    g_remaining_size = total_field_size
    flag_collect = True

    if common_type_dict.has_key( struct_name ):
        build_common_type_list( collect_list, total_field_size, struct_name )
    else:
        if ( name_structure_dict.has_key( struct_name ) ):
            node = name_structure_dict.get( struct_name )
            #node.show()
            collect_field_name( node, field_name, flag_collect, 0, collect_list, list(), list(), name_structure_dict ) 
            #print str( collect_list )
        else:
            print "struc name: %s doesn't exist" % struct_name
    return collect_list


def collect_trace_log_statement( all_c_file_list, c_file_name_num_dict, cpp_path, args_for_cpp ):
    """
        visit all c files to find out about trace log statements.
        If so, parse it and pick it up into the output list.
        return value is a list as following:
        [ ( file_num, line_num, data_size, struct_name, field_name, log_txt),
          ( file_num, line_num, data_size, struct_name, field_name, log_txt),
          ...
        ]

        file_num and line_num are digital
        data_size is in units of bit.
        struct_name, field_name and log_txt are string

    """

    global type_length_dict
#    ptn = re.compile( r'^\s*TRACE_LOG\s*\(.+?,(.+?),(.+?),(.+?)(,\s*"(.+)")?\)\s*;\s*$' )
    ptn = re.compile( r'^\s*TRACE_LOG\s*\(.+?,(.+)\)\s*;.*$' )
    trace_log_list = list()
    for fn in all_c_file_list:
        f = open( fn )
        '''
        file_num = c_file_name_num_dict[ os.path.basename( fn ).replace( ".c", "_c" ) ]
        '''
        line_num = 0
        for eachline in f:
            line_num += 1
            ret = ptn.match( eachline )
            if ( ret != None ):
                #print "%s  %s  %s  %s" % ret.groups()
                # 0: only either digital or sizeof(...)
                # 1: struct name
                # 2: field name (optional, Not present, 0 or struct name means logging from the beginning)
                # 3: additional text (optional)
                '''
                not for all the c files.
                '''
                file_num = c_file_name_num_dict[ os.path.basename( fn ).replace( ".c", "_c" ) ]
                token_list = ret.groups()[ 0 ].split( "," )
                data_size  = token_list[ 0 ].translate( None, " " )
                if ( data_size[0:6] == "sizeof" ):
                    if False == type_length_dict.has_key( data_size[ 7:-1 ] ):
                        print "[Error] File %s Line %d: %s can't be parsed!" % ( f.name, line_num, data_size )
                        continue
                    else:
                        data_size = type_length_dict[ data_size[ 7:-1 ] ]
                else:
                    data_size = int( data_size ) * 8
                struct_name= token_list[ 1 ].strip()

                if ( len( token_list ) > 2 ):
                    field_name = token_list[ 2 ].translate( None, " " )
                    if ( field_name == "0" ):
                        field_name = struct_name
                    if ( len( token_list ) > 3 ):
                        log_txt = ",".join( token_list[ 3: ] ).strip()
                    else:
                        log_txt = '""'
                else:
                    field_name = struct_name
                    log_txt    = '""'
                trace_log_list.append( ( file_num, line_num, data_size, struct_name, field_name, log_txt ) )
                #print "%2d, %4d, %3d, %15s, %10s, %s" % ( file_num, line_num, data_size, struct_name, field_name, log_txt )

        f.close()

    ptnP = re.compile( r'^\s*TL_PERFORMANCE\s*\(.+?,?(.+)\)\s*;.*$' )
    for fn in all_c_file_list:
        f = open( fn )
        '''
        file_num = c_file_name_num_dict[ os.path.basename( fn ).replace( ".c", "_c" ) ]
        '''
        line_num = 0
        for eachline in f:
            line_num += 1
            ret = ptnP.match( eachline )
            if ( ret != None ):
                #print "%s  %s  %s  %s" % ret.groups()
                # 0: only either digital or sizeof(...)
                # 1: struct name
                # 2: field name (optional, Not present, 0 or struct name means logging from the beginning)
                # 3: additional text (optional)
                '''
                not for all the c files.
                '''
                #print "one match:aa%saa" %ret.groups()[ 0 ]
                file_num = c_file_name_num_dict[ os.path.basename( fn ).replace( ".c", "_c" ) ]
                token_list = ret.groups()[ 0 ].split( "," )
                data_size = 8 * 8
                struct_name= 'PTL_INFO'
                field_name = struct_name
                if ('"' in ret.groups()[ 0 ]):
                    log_txt = ",".join( token_list[ 1: ] ).strip()
                else:
                    log_txt = '""'
                trace_log_list.append( ( file_num, line_num, data_size, struct_name, field_name, log_txt ) )
                #print "%2d, %4d, %3d, %15s, %10s, %s" % ( file_num, line_num, data_size, struct_name, field_name, log_txt )

        f.close()


    return trace_log_list

def build_c_file_name_num_dict( node, c_file_name_num_dict, index = 0 ):
    """
    traverse node to find out all Enumerators.
    count it and append it into dict
    return dict.
    """
    pfn = build_c_file_name_num_dict
    t = type( node )
    if ( issubclass( t, c_ast.EnumeratorList ) ):
        for child in node.enumerators:
            index = pfn( child, c_file_name_num_dict, index )
            #print "%-25s  %3d" % ( child.name, index )
            index += 1
        ret = index
    elif ( issubclass( t, c_ast.Enumerator ) ):
        if ( node.value != None ):
            index = pfn( node.value, c_file_name_num_dict )
        c_file_name_num_dict[ node.name ] = index
        ret = index
    elif ( issubclass( t, ( c_ast.Typedef, c_ast.TypeDecl ) ) ):
        ret = pfn( node.type, c_file_name_num_dict, index )
    elif ( issubclass( t, c_ast.Enum ) ):
        ret = pfn( node.values, c_file_name_num_dict, index )
    elif ( issubclass( t, c_ast.Constant ) ):
        ret = int( node.value )
    elif ( issubclass( t, c_ast.BinaryOp ) ):
        left = pfn( node.left, c_file_name_num_dict )
        right = pfn( node.right, c_file_name_num_dict )
        ret = eval( "%d %s %d" % (left, node.op, right ) )
    else:
        ret = 0

    return ret

def delete_temple_files(temple_dir):
    '''
    delete all the temple files
    '''
    shutil.rmtree(temple_dir)
    
def process():
    """
        the main procedure function
    """
    global name_structure_dict
    global type_length_dict
    
# 1. parse a *.ini file to import parameters from the file.
    print "Parse configuration file ..."
    ret =  parse_args_preprocess()
    if ( ret[ 0 ] == False ):
        # failed to parse the configuration file.
        exit()
    else:
        ( ret_val, trace_log_file, all_c_file_list, all_h_file_list, cpp_path, args_for_cpp,
          c_fn_enum_name, output_log_format_file, temple_dir) = ret

# 2. find out the specified trace log file (default: Trace_Log.c)
    name_structure_dict = dict()
    #print "Parse the trace log include file (%s) ..." % trace_log_file
    parse_trace_log_c_file( trace_log_file, cpp_path, args_for_cpp, name_structure_dict, type_length_dict)

# 3. find out enum "C_FILE_NAME_ENUM", build a dict for FileName --> FileNum.
    if ( True == name_structure_dict.has_key( c_fn_enum_name ) ):
        c_file_name_num_dict = dict()
        print "Build the file name enum  ..."
        build_c_file_name_num_dict( name_structure_dict.get( c_fn_enum_name ), c_file_name_num_dict )

# 4. collect "TRACE_LOG" statements in "all_c_file_list"
        print "Parse all C files ..."
        trace_log_statement_list = collect_trace_log_statement( all_c_file_list, c_file_name_num_dict, cpp_path, args_for_cpp )
        print "Summary"
        print "--------------------"
        print "\t%3d *.c files" % len( all_c_file_list )
        print "\t%3d name-structs" % len( name_structure_dict )
        print "\t%3d type-length" % len( type_length_dict )
        print "\t%3d TRACE_LOG calls" % len( trace_log_statement_list )
        if len( trace_log_statement_list ):
# 5. build log_format_file for each "TRACE_LOG" statement.
            print "Parser all TraceLog calls ..."
            decode_trace_log_statement( output_log_format_file, trace_log_statement_list )
            print "\nGenerate the log format file OK!"
            delete_temple_files(temple_dir)
    else:
        print "No enumeration definition for all C files\nWithout that, the parser can't continue to process"

if __name__ == "__main__":
    process()
