# Copyright 2014 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

"""
Parse gl.xml into Python objects.
"""

import os.path
import re
import sys
import xml.etree.cElementTree as ElementTree
from textwrap import dedent

# Export 'debug' so other Piglit modules can easily enable it.
debug = False

def _log_debug(msg):
    if debug:
        sys.stderr.write('debug: {0}: {1}\n'.format(__name__, msg))

def parse():
    """Parse gl.xml and return a Registry object."""
    filename = os.path.join(os.path.dirname(__file__), 'gl.xml')
    elem = ElementTree.parse(filename)
    xml_registry = elem.getroot()
    return Registry(xml_registry)

class OrderedKeyedSet(object):
    """A set with keyed elements that preserves order of element insertion.

    Why waste words? Let's document the class with example code.

    Example:
        Cheese = namedtuple('Cheese', ('name', 'flavor'))
        cheeses = OrderedKeyedSet(key='name')
        cheeses.add(Cheese(name='cheddar', flavor='good'))
        cheeses.add(Cheese(name='gouda', flavor='smells like feet'))
        cheeses.add(Cheese(name='romano', flavor='awesome'))

        # Elements are retrievable by key.
        assert(cheeses['gouda'].flavor == 'smells like feet')

        # On key collision, the old element is removed.
        cheeses.add(Cheese(name='gouda', flavor='ok i guess'))
        assert(cheeses['gouda'].flavor == 'ok i guess')

        # The set preserves order of insertion. Replacement does not alter
        # order.
        assert(list(cheeses)[2].name == 'romano')

        # The set is iterable.
        for cheese in cheeses:
            print(cheese.name)

        # Yet another example...
        Bread = namedtuple('Bread', ('name', 'smell'))
        breads = OrderedKeyedSet(key='name')
        breads.add(Bread(name='como', smell='subtle')
        breads.add(Bread(name='sourdough', smell='pleasant'))

        # The set supports some common set operations, such as union.
        breads_and_cheeses = breads | cheeses
        assert(len(breads_and_cheeses) == len(breads) + len(cheeses))
    """

    def __init__(self, key, elems=()):
        """Create a new set with the given key.

        The given 'key' defines how to calculate each element's key value.  If
        'key' is a string, then each key value is defined to be `getattr(elem, key)`.
        If 'key' is a function, then the key value is `key(elem)`.
        """
        if isinstance(key, str):
            self.__key_func = lambda elem: getattr(elem, key)
        else:
            self.__key_func = key

        self.__key_list = []
        self.__dict = dict()

        for e in elems:
            self.add(e)

    def __or__(x, y):
        """Same as `union`."""
        return x.union(y)

    def __contains__(self, key):
        return key in self.__dict

    def __delitem__(self, key):
        self.pop(key)
        return None

    def __getitem__(self, key):
        return self.__dict[key]

    def __iter__(self):
        return self.__dict.itervalues()

    def __len__(self):
        return len(self.__key_list)

    def __repr__(self):
        return '{0}({1})'.format(self.__class__.__name__, self.__key_list)

    def copy(self):
        """Return shallow copy."""
        other = OrderedKeyedSet(key=self.__key_func)
        other.__key_list = [k for k in self.__key_list]
        other.__dict = self.__dict.copy()
        return other

    def add(self, x):
        key = self.__key_func(x)
        if key not in self.__dict:
            self.__key_list.append(key)
        self.__dict[key] = x

    def clear(self):
        self.__key_list = []
        self.__dict = dict()

    def extend(self, elems):
        for e in elems:
            self.add(e)

    def get(self, key, default):
        return self.__dict.get(key, default)

    def get_key(self, x):
        return self.__key_func(x)

    def iteritems(self, x):
        for key in self.__key_list:
            yield (key, self.__dict[key])

    def iterkeys(self):
        return iter(self.__key_list)

    def itervalues(self):
        for key in self.__key_list:
            yield self.__dict[key]

    def pop(self, key):
        # This function should raise Exceptions similar to dict.pop, so call
        # dict.pop first.
        value = self.__dict.pop(key)
        self.__key_list.remove(key)
        return value

    def sort_by_key(self):
        self.__key_list.sort()

    def sort_by_value(self):
        def cmp(key1, key2):
            return cmp(self.__dict[key1], self.__dict[key2])
        self.__key_list.sort(cmp=cmp)

    def union(self, other):
        """Return the union of two sets as a new set.

        The the new set is ordered so that all elements of self precede those
        of other, and the order of elements within each origin set is
        preserved.
        
        The new set's key function is taken from self.  On key collisions, set
        y has precedence over x.
        """
        u = self.copy()
        u.extend(other)
        return u

class Registry(object):
    """The toplevel <registry> element.

    Attributes:
        features: An OrderedKeyedSet of class Feature, where each element
        represents a <feature> element.

        extensions: An OrderedKeyedSet of class Extension, where each element
        represents a <extension> element.

        commands: An OrderedKeyedSet of class Command, where each element
        represents a <command> element.

        enum_groups: An ordered collection of class EnumGroup, where each
        element represents an <enums> element.

        enums: An OrderedKeyedSet of class Enum, where each element
        represents an <enum> element.

        vendor_namespaces: A collection of all vendor prefixes and suffixes.
        For example, "ARB", "EXT", "CHROMIUM", "NV".
    """

    def __init__(self, xml_registry):
        """Parse the <registry> element."""

        assert(xml_registry.tag == 'registry')

        self.features = OrderedKeyedSet(
            key='name', elems=(
                Feature(xml_feature)
                for xml_feature in xml_registry.iterfind('./feature')
            )
        )

        self.extensions = OrderedKeyedSet(
            key='name', elems=(
                Extension(xml_extension)
                for xml_extension in xml_registry.iterfind('./extensions/extension')
            )
        )

        self.commands = OrderedKeyedSet(
            key='name', elems=(
                Command(xml_command)
                for xml_command in xml_registry.iterfind('./commands/command')
            )
        )

        self.enum_groups = [
            EnumGroup(xml_enums)
            for xml_enums in xml_registry.iterfind('./enums')
        ]

        self.__fix_enum_groups()

        self.enums = OrderedKeyedSet(
            key='name', elems=(
                enum
                for group in self.enum_groups
                for enum in group.enums
                if not enum.is_collider
            )
        )

        self.vendor_namespaces = {
            extension.vendor_namespace
            for extension in self.extensions
            if extension.vendor_namespace is not None
        }

        for feature in self.features:
            feature._link_commands(self.commands)
            feature._link_enums(self.enums)

        for extension in self.extensions:
            extension._link_commands(self.commands)
            extension._link_enums(self.enums)

        self.__set_command_name_parts()

    def __fix_enum_groups(self):
        for enum_group in self.enum_groups:
            if (enum_group.name is None
                and enum_group.vendor == 'SGI'
                and enum_group.start == '0x8000'
                and enum_group.end == '0x80BF'):
                   self.enum_groups.remove(enum_group)
            elif (enum_group.name is None
                  and enum_group.vendor == 'ARB'
                  and enum_group.start == None
                  and enum_group.end == None):
                     enum_group.start = '0x8000'
                     enum_group.start = '0x80BF'

    def __set_command_name_parts(self):
        suffix_choices = '|'.join(self.vendor_namespaces)
        regex = '^(?P<basename>[a-zA-Z0-9_]+?)(?P<vendor_suffix>{suffix_choices})?$'
        regex = regex.format(suffix_choices=suffix_choices)
        _log_debug('setting command name parts with regex {0!r}'.format(regex))
        regex = re.compile(regex)
        for command in self.commands:
            command._set_name_parts(regex)

class Feature(object):
    """A <feature> XML element.

    Attributes:
        name: The XML element's 'name' attribute.

        api: The XML element's 'api' attribute.

        version_str:  The XML element's 'number' attribute. For example, "3.1".

        version_float: float(version_str)

        version_int: int(10 * version_float)

        commands: An OrderedKeyedSet of class Command that contains all
            <command> subelements.

        enums: An OrderedKeyedSet of class Enum that contains all <enum>
            subelements.
    """

    def __init__(self, xml_feature):
        """Parse a <feature> element."""

        # Example <feature> element:
        #
        #    <feature api="gles2" name="GL_ES_VERSION_3_1" number="3.1">
        #        <!-- arrays_of_arrays features -->
        #        <require/>
        #        <!-- compute_shader features -->
        #        <require>
        #            <command name="glDispatchCompute"/>
        #            <command name="glDispatchComputeIndirect"/>
        #            <enum name="GL_COMPUTE_SHADER"/>
        #            <enum name="GL_MAX_COMPUTE_UNIFORM_BLOCKS"/>
        #            ...
        #        </require>
        #        <!-- draw_indirect features -->
        #        <require>
        #            <command name="glDrawArraysIndirect"/>
        #            <command name="glDrawElementsIndirect"/>
        #            <enum name="GL_DRAW_INDIRECT_BUFFER"/>
        #            <enum name="GL_DRAW_INDIRECT_BUFFER_BINDING"/>
        #        </require>
        #        ...
        #    </feature>

        assert(xml_feature.tag == 'feature')
        self._xml_feature = xml_feature

        # Parse the <feature> tag's attributes.
        self.api = xml_feature.get('api')
        self.name = xml_feature.get('name')

        self.version_str = xml_feature.get('number')
        self.version_float = float(self.version_str)
        self.version_int = int(10 * self.version_float)

        self.commands = OrderedKeyedSet(key='name')
        self.enums = OrderedKeyedSet(key='name')

    def _link_commands(self, commands):
        """Parse <command> subelements and link them to the Command objects in
        'commands'.
        """
        for xml_command in self._xml_feature.iterfind('./require/command'):
            command_name = xml_command.get('name')
            command = commands[command_name]
            assert(isinstance(command, Command))
            _log_debug('link command {0!r} and feature {1!r})'.format(command.name, self.name))
            self.commands.add(command)
            command.features.add(self)

    def _link_enums(self, enums):
        """Parse <enum> subelements and link them to the Command objects in
        'enums'.
        """
        for xml_enum in self._xml_feature.iterfind('./require/enum'):
            enum_name = xml_enum.get('name')
            enum = enums[enum_name]
            assert(isinstance(enum, Enum))
            _log_debug('link command {0!r} and feature {1!r}'.format(enum.name, self.name))
            self.enums.add(enum)
            enum.features.add(self)

class Extension(object):
    """An <extension> XML element.

    Attributes:
        name: The XML element's 'name' attribute.

        supported_apis: The collection of api strings in the XML element's
            'supported' attribute. For example, ['gl', 'glcore'].

        vendor_namespace: For example, "AMD". May be None.

        commands: An OrderedKeyedSet of class Command that contains all
            <command> subelements.

        enums: An OrderedKeyedSet of class Enum that contains all <enum>
            subelements.
    """

    __vendor_regex = re.compile(r'^GL_(?P<vendor_namespace>[A-Z]+)_')

    def __init__(self, xml_extension):
        """Parse an <extension> element."""

        # Example <extension> element:
        #     <extension name="GL_ARB_ES2_compatibility" supported="gl|glcore">
        #         <require>
        #             <enum name="GL_FIXED"/>
        #             <enum name="GL_IMPLEMENTATION_COLOR_READ_TYPE"/>
        #             ...
        #             <command name="glReleaseShaderCompiler"/>
        #             <command name="glShaderBinary"/>
        #             ...
        #         </require>
        #     </extension>

        assert(xml_extension.tag == 'extension')
        self._xml_extension = xml_extension

        # Parse the <extension> tag's attributes.
        self.name = xml_extension.get('name')
        self.supported_apis = xml_extension.get('supported').split('|')

        self.vendor_namespace = None
        match = Extension.__vendor_regex.match(self.name)
        if match is not None:
            self.vendor_namespace = match.groupdict().get('vendor_namespace', None)

        self.commands = OrderedKeyedSet(key='name')
        self.enums = OrderedKeyedSet(key='name')

    def __cmp__(x, y):
        """Compare by name."""
        return cmp(x.name, y.name)

    def _link_commands(self, commands):
        """Parse <command> subelements and link them to the Command objects in
        'commands'.
        """
        for xml_command in self._xml_extension.iterfind('./require/command'):
            command_name = xml_command.get('name')
            command = commands[command_name]
            assert(isinstance(command, Command))
            _log_debug('link command {0!r} and extension {1!r}'.format(command.name, self.name))
            self.commands.add(command)
            command.extensions.add(self)

    def _link_enums(self, enums):
        """Parse <enum> subelements and link them to the Command objects in
        'enums'.
        """
        for xml_enum in self._xml_extension.iterfind('./require/enum'):
            enum_name = xml_enum.get('name')
            enum = enums[enum_name]
            assert(isinstance(enum, Enum))
            _log_debug('link command {0!r} and extension {1!r}'.format(enum.name, self.name))
            self.enums.add(enum)
            enum.extensions.add(self)

class CommandParam(object):
    """A <param> XML element at path command/param.

    Attributes:
        name
        c_type
    """
    
    __PARAM_NAME_FIXES = {'near': 'hither', 'far': 'yon'}

    def __init__(self, xml_param, log=None):
        """Parse a <param> element."""

        # Example <param> elements:
        #
        #    <param>const <ptype>GLchar</ptype> *<name>name</name></param>
        #    <param len="1"><ptype>GLsizei</ptype> *<name>length</name></param>
        #    <param len="bufSize"><ptype>GLint</ptype> *<name>values</name></param>
        #    <param><ptype>GLenum</ptype> <name>shadertype</name></param>
        #    <param group="sync"><ptype>GLsync</ptype> <name>sync</name></param>

        assert(xml_param.tag == 'param')

        self.name = xml_param.find('./name').text

        # Rename the parameter if its name is a reserved keyword in MSVC.
        self.name =  self.__PARAM_NAME_FIXES.get(self.name, self.name)

        # Pare the C type.
        c_type_text = list(xml_param.itertext())
        c_type_text.pop(-1) # Pop off the text from the <name> subelement.
        c_type_text = (t.strip() for t in c_type_text)
        self.c_type = ' '.join(c_type_text).strip()

        _log_debug('parsed {0}'.format(self))

    def __repr__(self):
        templ = (
            '{self.__class__.__name__}('
            'name={self.name!r}, '
            'type={self.c_type!r})')
        return templ.format(self=self)
        
class Command(object):
    """A <command> XML element.

    Attributes:
        name: The XML element's 'name' attribute.

        features: An OrderedKeyedSet of class Feature that enumerates all
            features that require this command.

        extensions: An OrderedKeyedSet of class Extensions that enumerates all
            extensions that require this command.

        c_return_type: For example, "void *".

        alias: The XML element's 'alias' element. May be None.

        param_list: List of class CommandParam that contains all <param>
            subelements.
    """
    
    def __init__(self, xml_command):
        """Parse a <command> element."""

        # Example <command> element:
        #
        #    <command>
        #        <proto>void <name>glTexSubImage2D</name></proto>
        #        <param group="TextureTarget"><ptype>GLenum</ptype> <name>target</name></param>
        #        <param group="CheckedInt32"><ptype>GLint</ptype> <name>level</name></param>
        #        <param group="CheckedInt32"><ptype>GLint</ptype> <name>xoffset</name></param>
        #        <param group="CheckedInt32"><ptype>GLint</ptype> <name>yoffset</name></param>
        #        <param><ptype>GLsizei</ptype> <name>width</name></param>
        #        <param><ptype>GLsizei</ptype> <name>height</name></param>
        #        <param group="PixelFormat"><ptype>GLenum</ptype> <name>format</name></param>
        #        <param group="PixelType"><ptype>GLenum</ptype> <name>type</name></param>
        #        <param len="COMPSIZE(format,type,width,height)">const void *<name>pixels</name></param>
        #        <glx type="render" opcode="4100"/>
        #        <glx type="render" opcode="332" name="glTexSubImage2DPBO" comment="PBO protocol"/>
        #    </command>
        #

        assert(xml_command.tag == 'command')
        self.__xml_command = xml_command

        xml_proto = xml_command.find('./proto')
        self.name = xml_proto.find('./name').text
        _log_debug('start parsing Command(name={0!r})'.format(self.name))

        self.features = OrderedKeyedSet(key='name')
        self.extensions = OrderedKeyedSet(key='name')

        # Parse the return type from the <proto> element.
        #
        # Example of a difficult <proto> element:
        #     <proto group="String">const <ptype>GLubyte</ptype> *<name>glGetStringi</name></proto>
        c_return_type_text = list(xml_proto.itertext())
        c_return_type_text.pop(-1) # Pop off the text from the <name> subelement.
        c_return_type_text = (t.strip() for t in c_return_type_text)
        self.c_return_type = ' '.join(c_return_type_text).strip()

        # Parse alias info, if any.
        xml_alias = xml_command.find('./alias')
        if xml_alias is None:
            self.alias = None
        else:
            self.alias = xml_alias.get('name')

        self.param_list = [
            CommandParam(xml_param)
            for xml_param in xml_command.iterfind('./param')
        ]

        _log_debug(('parsed {self.__class__.__name__}('
                     'name={self.name!r}, '
                     'alias={self.alias!r}, '
                     'prototype={self.c_prototype!r}, '
                     'features={self.features}, '
                     'extensions={self.extensions})').format(self=self))

    def __cmp__(x, y):
        return cmp(x.name, y.name)

    def __repr__(self):
        cls = self.__class__
        return '{cls.__name__}({self.name!r})'.format(**locals())

    @property
    def c_prototype(self):
        """For example, "void glAccum(GLenum o, GLfloat value)"."""
        return '{self.c_return_type} {self.name}({self.c_named_param_list})'.format(self=self)

    @property
    def c_funcptr_typedef(self):
        """For example, "PFNGLACCUMROC" for glAccum."""
        return 'PFN{0}PROC'.format(self.name).upper()

    @property
    def c_named_param_list(self):
        """For example, "GLenum op, GLfloat value" for glAccum."""
        return ', '.join([
            '{param.c_type} {param.name}'.format(param=param)
            for param in self.param_list
        ])

    @property
    def c_unnamed_param_list(self):
        """For example, "GLenum, GLfloat" for glAccum."""
        return ', '.join([
            param.c_type
            for param in self.param_list
        ])

    @property
    def c_untyped_param_list(self):
        """For example, "op, value" for glAccum."""
        return ', '.join([
            param.name
            for param in self.param_list
        ])

    @property
    def requirements(self):
        """The union of features and extensions that require this commnand."""
        return self.features | self.extensions

    def _set_name_parts(self, regex):
        groups = regex.match(self.name).groupdict()
        self.basename = groups['basename']
        self.vendor_suffix = groups.get('vendor_suffix', None)

class EnumGroup(object):
    """An <enums> element at path registry/enums.

    Attributes:
        name: The XML element's 'group' attribute. If the XML does not define
            'group', then this class invents one.

        type: The XML element's 'type' attribute. If the XML does not define
            'type', then this class invents one.

        start, end: The XML element's 'start' and 'end' attributes. May be
            None.

        enums: An OrderedKeyedSet of class Enum that contains all <enum>
            sublements.
    """

    # Each EnumGroup belongs to exactly one member of EnumGroup.TYPES.
    #
    # Some members in EnumGroup.TYPES are invented and not present in gl.xml.
    # The only enum type defined explicitly in gl.xml is "bitmask", which
    # occurs as <enums type="bitmask">.  However, in gl.xml each block of
    # non-bitmask enums is introduced by a comment that describes the block's
    # "type", even if the <enums> tag lacks a 'type' attribute. (Thanks,
    # Khronos, for encoding data in XML comments rather than the XML itself).
    # EnumGroup.TYPES lists such implicit comment-only types, with invented
    # names, alongside the types explicitly defined by <enums type=>.
    TYPES = (
        # Type 'default_namespace' is self-explanatory. It indicates the large
        # set of enums from 0x0000 to 0xffff that includes, for example,
        # GL_POINTS and GL_TEXTURE_2D.
        'default_namespace',

        # Type 'bitmask' is self-explanatory.
        'bitmask',

        # Type 'small_index' indicates a small namespace of non-bitmask enums.
        # As of Khronos revision 26792, 'small_index' groups generally contain
        # small numbers used for indexed access.
        'small_index',

        # Type 'special' is used only for the group named "SpecialNumbers". The
        # group contains enums such as GL_FALSE, GL_ZERO, and GL_INVALID_INDEX.
        'special',
    )

    @staticmethod
    def __fix_xml(xml_elem):
        """Add missing attributes and fix misnamed ones."""
        if xml_elem.get('namespace') == 'OcclusionQueryEventMaskAMD':
            # This tag's attributes are totally broken.
            xml_elem.set('namespace', 'GL')
            xml_elem.set('group', 'OcclusionQueryEventMaskAMD')
            xml_elem.set('type', 'bitmask')
        elif xml_elem.get('vendor', None) == 'ARB':
            enums = xml_elem.findall('./enum')
            if (len(enums) != 0
                    and enums[0].get('value') == '0x8000'
                    and enums[-1].get('value') == '0x80BF'):
                # This tag lacks 'start' and 'end' attributes.
                xml_elem.set('start','0x8000')
                xml_elem.set('end', '0x80BF')

    def __init__(self, xml_enums):
        """Parse an <enums> element."""

        # Example of a bitmask group:
        #
        #     <enums namespace="GL" group="SyncObjectMask" type="bitmask">
        #         <enum value="0x00000001" name="GL_SYNC_FLUSH_COMMANDS_BIT"/>
        #         <enum value="0x00000001" name="GL_SYNC_FLUSH_COMMANDS_BIT_APPLE"/>
        #     </enums>
        #
        # Example of a group that resides in OpenGL's default enum namespace:
        #
        #     <enums namespace="GL" start="0x0000" end="0x7FFF" vendor="ARB" comment="...">
        #         <enum value="0x0000" name="GL_POINTS"/>
        #         <enum value="0x0001" name="GL_LINES"/>
        #         <enum value="0x0002" name="GL_LINE_LOOP"/>
        #         ...
        #     </enums>
        #
        # Example of a non-bitmask group that resides outside OpenGL's default
        # enum namespace:
        #     
        #     <enums namespace="GL" group="PathRenderingTokenNV" vendor="NV">
        #         <enum value="0x00" name="GL_CLOSE_PATH_NV"/>
        #         <enum value="0x02" name="GL_MOVE_TO_NV"/>
        #         <enum value="0x03" name="GL_RELATIVE_MOVE_TO_NV"/>
        #         ...
        #     </enums>
        #

        EnumGroup.__fix_xml(xml_enums)

        self.name = xml_enums.get('group', None)
        _log_debug(('start parsing {self.__class__.__name__}('
                    'name={self.name!r}').format(self=self))
        self.type = xml_enums.get('type', None)
        self.start = xml_enums.get('start', None)
        self.end = xml_enums.get('end', None)
        self.enums = []

        self.__invent_name_and_type()
        assert(self.name is not None)
        assert(self.type in self.TYPES)

        # Parse the <enum> subelements.
        _log_debug('start parsing subelements of {self}'.format(self=self))
        self.enums = OrderedKeyedSet(
            key='name', elems=(
                Enum(self, xml_enum)
                for xml_enum in xml_enums.iterfind('./enum')
            )
        )
        _log_debug('parsed {0}'.format(self))

    def __repr__(self):
        return (
            '{self.__class__.__name__}('
            'name={self.name!r}, '
            'type={self.type!r}, '
            'start={self.start}, '
            'end={self.end}, '
            'enums={enums})'
        ).format(
            self=self,
            enums= ', '.join([repr(e) for e in self.enums]),
        )

    def __invent_name_and_type(self):
        """If the XML didn't define a name or type, invent one."""
        if self.name is None:
            assert(self.type is None)
            assert(self.start is not None)
            assert(self.end is not None)
            self.name = 'range_{self.start}_{self.end}'.format(self=self)
            self.type = 'default_namespace'
        elif self.type is None:
            self.type = 'small_index'
        elif self.name == 'SpecialNumbers':
            assert(self.type is None)
            self.type = 'special'

    def __post_fix_xml(self):
        if (self.vendor == "ARB"
                and self.start is None
                and self.end is None
                and self.enums[0].num_value == 0x8000
                and self.enums[-1].num_value == 0x80BF):
            self.start = '0x8000'
            self.end = '0x0BF'

class Enum(object):
    """An <enum> XML element.

    Attributes:
        name, api: The XML element's 'name' and 'api' attributes.

        str_value, c_num_literal: Equivalent attributes. The XML element's
            'value' attribute.

        num_value: The long integer for str_value.

        features: An OrderedKeyedSet of class Feature that enumerates all
            features that require this enum.

        extensions: An OrderedKeyedSet of class Extensions that enumerates all
            extensions that require this enum.
    """

    def __init__(self, enum_group, xml_enum):
        """Parse an <enum> tag located at path registry/enums/enum."""

        # Example <enum> element:
        #     <enum value="0x0000" name="GL_POINTS"/>

        assert(isinstance(enum_group, EnumGroup))
        assert(xml_enum.tag == 'enum')

        self.group = enum_group
        self.name = xml_enum.get('name')
        self.api = xml_enum.get('api')
        self.str_value = xml_enum.get('value')
        self.features = OrderedKeyedSet(key='name')
        self.extensions = OrderedKeyedSet(key='name')
        self.c_num_literal = self.str_value

        if '0x' in self.str_value.lower():
            base = 16
        else:
            base = 10
        self.num_value = long(self.str_value, base)

        _log_debug('parsed {0}'.format(self))

    def __repr__(self):
        return ('{self.__class__.__name__}('
                'name={self.name!r}, '
                'value={self.str_value!r}, '
                'features={self.features}, '
                'extensions={self.extensions})').format(self=self)

    @property
    def is_collider(self):
        """Does the enum's name collide with another enum's name?"""
        if self.name == 'GL_ACTIVE_PROGRAM_EXT' and self.api == 'gles2':
            # This enum has different numerical values in GL (0x8B8D) and
            # in GLES (0x8259).
            return True
        else:
            return False

    @property
    def requirements(self):
        return self.features | self.extensions
