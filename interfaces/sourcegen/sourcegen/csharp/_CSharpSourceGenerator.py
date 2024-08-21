# This file is part of Cantera. See License.txt in the top-level directory or
# at https://cantera.org/license.txt for license and copyright information.

from pathlib import Path
import sys
import logging
from typing import List, Dict
import re

from jinja2 import Environment, BaseLoader

from ._dataclasses import CsFunc
from ._Config import Config
from .._dataclasses import Func, Param, HeaderFile, ArgList
from .._SourceGenerator import SourceGenerator


_logger = logging.getLogger()
_loader = Environment(loader=BaseLoader)

class CSharpSourceGenerator(SourceGenerator):
    """The SourceGenerator for scaffolding C# files for the .NET interface"""

    def _preamble(self, file_name: str) -> str:
        template = _loader.from_string(self._templates["csharp-preamble"])
        return template.render(file_name=file_name)

    def _get_property_text(self, clib_area: str, c_name: str, cs_name: str,
                           known_funcs: Dict[str, CsFunc]) -> str:
        getter = known_funcs.get(clib_area + "_" + c_name)

        if getter:
            # here we have found a simple scalar property
            prop_type = getter.ret_type
        else:
            # here we have found an array-like property (string, double[])
            getter = known_funcs[clib_area + "_get" + c_name.capitalize()]
            # this assumes the last param in the function is a pointer type,
            # from which we determine the appropriate C# type
            prop_type = self._config.prop_type_crosswalk[getter.arglist[-1].p_type]

        setter = known_funcs.get(
            clib_area + "_set" + c_name.capitalize(),
            CsFunc("", "", "", "", ""))

        if prop_type in ["int", "double"]:
            template = _loader.from_string(self._templates["csharp-property-int-double"])
            text = template.render(
                prop_type=prop_type, cs_name=cs_name,
                getter=getter.name, setter=setter.name)
        elif prop_type == "string":
            # for get-string type functions we need to look up the type of the second
            # (index 1) param for a cast because sometimes it"s an int and other times
            # its a nuint (size_t)
            template = _loader.from_string(self._templates["csharp-property-string"])
            text = template.render(
                cs_name=cs_name, p_type=getter.arglist[1].p_type,
                getter=getter.name, setter=setter.name)
        else:
            _logger.critical(f"Unable to scaffold properties of type {prop_type!r}!")
            sys.exit(1)

        return text

    def __init__(self, out_dir: str, config: dict, templates: dict):
        if not out_dir:
            _logger.critical("Non-empty string identifying output path required.")
            sys.exit(1)
        self._out_dir = Path(out_dir)

        # use the typed config
        self._config = Config.from_parsed(**config)
        self._templates = templates

    def _get_wrapper_class_name(self, clib_area: str) -> str:
        return self._config.class_crosswalk[clib_area]

    def _get_handle_class_name(self, clib_area: str) -> str:
        return self._get_wrapper_class_name(clib_area) + "Handle"

    def _convert_func(self, parsed: Func) -> CsFunc:
        ret_type, name, _ = parsed
        clib_area, method = name.split("_", 1)

        # Shallow copy the params list
        # Some of the C# params will have the same syntax as the C params.
        # Others will be represented differently on the C# side, and we will
        # replace their entry in the list.
        # Therefore, copy the list so that we don’t accidentally modify
        # the params list which is attached to the C func.
        params = parsed.arglist[:]

        release_func_handle_class_name = None

        if clib_area != "ct":
            handle_class_name = self._get_handle_class_name(clib_area)

            # It’s not a “global” function, therefore:
            #   * It wraps a constructor and returns a handle,
            #   * It wraps an instance method that returns a handle, or
            #   * It wraps an instance method and takes the handle as the first param.
            if method.startswith("del"):
                release_func_handle_class_name = handle_class_name
            elif method.startswith("new"):
                ret_type = handle_class_name
            elif name in self._config.class_accessors:
                ret_type = self._config.class_accessors[name]
                params[0] = Param(handle_class_name, params[0].name)
            elif params:
                params[0] = Param(handle_class_name, params[0].name)

        for c_type, cs_type in self._config.ret_type_crosswalk.items():
            if ret_type == c_type:
                ret_type = cs_type
                break

        setter_double_arrays_count = 0

        for i, param in enumerate(params):
            param_type = param.p_type
            param_name = param.name

            for c_type, cs_type in self._config.ret_type_crosswalk.items():
                if param_type == c_type:
                    param_type = cs_type
                    break

            # Most "setter" functions for arrays in CLib use a const double*,
            # but we also need to handle the cases for a plain double*
            if param_type == "double*" and method.startswith("set"):
                setter_double_arrays_count += 1
                if setter_double_arrays_count > 1:
                    # We assume a double* can reliably become a double[].
                    # However, this logic is too simplistic if there is
                    # more than one array.
                    _logger.critical(f"Cannot scaffold {name!r} with "
                                     "more than one array of doubles!")
                    sys.exit(1)

                if clib_area == "thermo" and re.match("^set_[A-Z]{2}$", method):
                    # Special case for the functions that set thermo pairs
                    # This allows the C# side to pass a pointer to the stack
                    # Rather than allocating an array on the heap (which requires GC)
                    param_type = "(double, double)*"
                else:
                    param_type = "double[]"

            params[i] = Param(param_type, param_name)

        func = CsFunc(ret_type,
                      name,
                      ArgList(params),
                      release_func_handle_class_name is not None,
                      release_func_handle_class_name)

        return func

    def _write_file(self, filename: str, contents: str):
        _logger.info(f"  writing {filename!r}")

        self._out_dir.joinpath(filename).write_text(contents, encoding="utf-8")

    def _scaffold_interop(self, header_file_path: Path, cs_funcs: List[CsFunc]):
        template = _loader.from_string(self._templates["csharp-interop-func"])
        function_list = []
        for func in cs_funcs:
            function_list.append(
                template.render(unsafe=func.unsafe(), declaration=func.declaration()))

        file_name = "Interop.LibCantera." + header_file_path.name + ".g.cs"
        preamble = self._preamble(file_name)

        template = _loader.from_string(self._templates["csharp-scaffold-interop"])
        interop_text = template.render(
            preamble=preamble, cs_functions=function_list)

        self._write_file(file_name, interop_text)

    def _scaffold_handles(self, header_file_path: Path, handles: Dict[str, str]):
        template = _loader.from_string(self._templates["csharp-base-handle"])
        handle_list = []
        for class_name, release_func_name in handles.items():
            handle_list.append(template.render(
                class_name=class_name, release_func_name=release_func_name))

        file_name = "Interop.Handles." + header_file_path.name + ".g.cs"
        preamble = self._preamble(file_name)

        template = _loader.from_string(self._templates["csharp-scaffold-handles"])
        handles_text = template.render(
            preamble=preamble, cs_handles=handle_list)

        self._write_file(file_name, handles_text)

    def _scaffold_derived_handles(self):
        template = _loader.from_string(self._templates["csharp-derived-handle"])
        handle_list = []
        for derived_class_name, base_class_name in self._config.derived_handles.items():
            handle_list.append(template.render(
                derived_class_name=derived_class_name, base_class_name=base_class_name))

        file_name = "Interop.Handles.g.cs"
        preamble = self._preamble(file_name)

        template = _loader.from_string(self._templates["csharp-scaffold-handles"])
        derived_handles_text = template.render(
            preamble=preamble, cs_handles=handle_list)

        self._write_file(file_name, derived_handles_text)

    def _scaffold_wrapper_class(self, clib_area: str, props: Dict[str, str],
                                known_funcs: Dict[str, CsFunc]):
        wrapper_class_name = self._get_wrapper_class_name(clib_area)
        handle_class_name = self._get_handle_class_name(clib_area)

        property_list = [
            self._get_property_text(clib_area, c_name, cs_name, known_funcs)
                for (c_name, cs_name) in props.items()]

        file_name = wrapper_class_name + ".g.cs"
        preamble = self._preamble(file_name)

        template = _loader.from_string(self._templates["csharp-scaffold-wrapper-class"])
        wrapper_class_text = template.render(
            preamble=preamble,
            wrapper_class_name=wrapper_class_name, handle_class_name=handle_class_name,
            cs_properties=property_list)

        self._write_file(file_name, wrapper_class_text)

    def generate_source(self, headers_files: List[HeaderFile]):
        self._out_dir.mkdir(parents=True, exist_ok=True)

        known_funcs: Dict[str, List[CsFunc]] = {}

        for header_file in headers_files:
            cs_funcs = list(map(self._convert_func, header_file.funcs))
            known_funcs.update((f.name, f) for f in cs_funcs)

            self._scaffold_interop(header_file.path, cs_funcs)

            handles = {func.handle_class_name: func.name
                for func in cs_funcs if func.is_handle_release_func}

            if not handles:
                continue

            self._scaffold_handles(header_file.path, handles)

        self._scaffold_derived_handles()

        for (clib_area, props) in self._config.wrapper_classes.items():
            self._scaffold_wrapper_class(clib_area, props, known_funcs)
