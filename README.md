utdataflow
==========
This is the utdataflow Ubitrack submodule.

Description
----------
The utdataflow contains frontend adapters for ubitrack dataflow networks. It also contains the utConsole.

Usage
-----
In order to use it, you have to clone the buildenvironment, change to the ubitrack directory and add the utfacade by executing:

    git submodule add https://github.com/Ubitrack/utdataflow.git modules/utdataflow


Dependencies
----------
In addition, this module has to following submodule dependencies which have to be added for successful building:

<table>
  <tr>
    <th>Dependency</th><th>Dependent Components</th><th>optional Dependency</th>
  </tr>
  <tr>
    <td>utcore</td><td>utDataflow</td><td>no</td>
  </tr>
</table>

