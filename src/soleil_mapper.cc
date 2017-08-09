/* Copyright 2017 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "soleil_mapper.h"

#include "default_mapper.h"

using namespace Legion;
using namespace Legion::Mapping;

static LegionRuntime::Logger::Category log_soleil("soleil");

class SoleilMapper : public DefaultMapper
{
public:
  SoleilMapper(MapperRuntime *rt, Machine machine, Processor local,
                const char *mapper_name,
                std::vector<Processor>* loc_procs_list,
                std::vector<Processor>* toc_procs_list,
                std::vector<Processor>* io_procs_list,
                std::vector<Processor>* omp_procs_list,
                std::vector<Memory>* sysmems_list,
                std::vector<Memory>* fbmems_list,
                std::map<Memory, std::vector<Processor> >* sysmem_local_procs,
                std::map<Memory, std::vector<Processor> >* sysmem_local_io_procs,
                std::map<Memory, std::vector<Processor> >* sysmem_local_omp_procs,
                std::map<Memory, std::vector<Processor> >* fbmem_local_procs,
                std::map<Processor, Memory>* proc_sysmems,
                std::map<Processor, Memory>* proc_regmems,
                std::map<Processor, Memory>* proc_fbmems,
                std::map<Processor, Memory>* proc_zcmems);
  virtual Processor default_policy_select_initial_processor(
                                    MapperContext ctx, const Task &task);
  virtual void default_policy_select_target_processors(
                                    MapperContext ctx,
                                    const Task &task,
                                    std::vector<Processor> &target_procs);
  virtual LogicalRegion default_policy_select_instance_region(
                                     MapperContext ctx, Memory target_memory,
                                     const RegionRequirement &req,
                                     const LayoutConstraintSet &constraints,
                                     bool force_new_instances,
                                     bool meets_constraints);
  virtual void slice_task(const MapperContext      ctx,
                          const Task&              task,
                          const SliceTaskInput&    input,
                                SliceTaskOutput&   output);
  virtual void map_task(const MapperContext      ctx,
                        const Task&              task,
                        const MapTaskInput&      input,
                              MapTaskOutput&     output);
  virtual void map_copy(const MapperContext ctx,
                        const Copy &copy,
                        const MapCopyInput &input,
                        MapCopyOutput &output);

  virtual void map_must_epoch(const MapperContext           ctx,
                              const MapMustEpochInput&      input,
                                    MapMustEpochOutput&     output);

protected:
  bool soleil_create_custom_instances(MapperContext ctx,
                          Processor target, Memory target_memory,
                          const RegionRequirement &req, unsigned index,
                          std::set<FieldID> &needed_fields, // will destroy
                          const TaskLayoutConstraintSet &layout_constraints,
                          bool needs_field_constraint_check,
                          std::vector<PhysicalInstance> &instances);
  template<bool IS_SRC>
  void soleil_create_copy_instance(MapperContext ctx, const Copy &copy,
                                   const RegionRequirement &req, unsigned index,
                                   std::vector<PhysicalInstance> &instances);
  Color get_task_color(MapperContext ctx, const Task &task, unsigned idx = 0);
private:
  bool use_gpu;
  bool use_omp;
  std::vector<Processor>& loc_procs_list;
  std::vector<Processor>& toc_procs_list;
  std::vector<Processor>& io_procs_list;
  std::vector<Processor>& omp_procs_list;
  std::vector<Memory>& sysmems_list;
  std::vector<Memory>& fbmems_list;
  std::map<Memory, std::vector<Processor> >& sysmem_local_procs;
  std::map<Memory, std::vector<Processor> >& sysmem_local_io_procs;
  std::map<Memory, std::vector<Processor> >& sysmem_local_omp_procs;
  std::map<Memory, std::vector<Processor> >& fbmem_local_procs;
  std::map<Processor, Memory>& proc_sysmems;
  std::map<Processor, Memory>& proc_regmems;
  std::map<Processor, Memory>& proc_fbmems;
  std::map<Processor, Memory>& proc_zcmems;

  std::vector<TaskSlice> slice_cache_compute;
  std::vector<TaskSlice> slice_cache_render;
  typedef std::vector<PhysicalInstance> CachedRegionMapping;
  typedef std::vector<CachedRegionMapping> CachedTaskMapping;
  std::map<std::pair<Color, RegionTreeID>, CachedTaskMapping> mapping_cache_render;
  std::map<Color, CachedRegionMapping> mapping_cache_particle;
};

//--------------------------------------------------------------------------
SoleilMapper::SoleilMapper(MapperRuntime *rt, Machine machine, Processor local,
                             const char *mapper_name,
                             std::vector<Processor>* _loc_procs_list,
                             std::vector<Processor>* _toc_procs_list,
                             std::vector<Processor>* _io_procs_list,
                             std::vector<Processor>* _omp_procs_list,
                             std::vector<Memory>* _sysmems_list,
                             std::vector<Memory>* _fbmems_list,
                             std::map<Memory, std::vector<Processor> >* _sysmem_local_procs,
                             std::map<Memory, std::vector<Processor> >* _sysmem_local_io_procs,
                             std::map<Memory, std::vector<Processor> >* _sysmem_local_omp_procs,
                             std::map<Memory, std::vector<Processor> >* _fbmem_local_procs,
                             std::map<Processor, Memory>* _proc_sysmems,
                             std::map<Processor, Memory>* _proc_regmems,
                             std::map<Processor, Memory>* _proc_fbmems,
                             std::map<Processor, Memory>* _proc_zcmems)
//--------------------------------------------------------------------------
  : DefaultMapper(rt, machine, local, mapper_name),
    use_gpu(false),
    use_omp(false),
    loc_procs_list(*_loc_procs_list),
    toc_procs_list(*_toc_procs_list),
    io_procs_list(*_io_procs_list),
    omp_procs_list(*_omp_procs_list),
    sysmems_list(*_sysmems_list),
    fbmems_list(*_fbmems_list),
    sysmem_local_procs(*_sysmem_local_procs),
    sysmem_local_io_procs(*_sysmem_local_io_procs),
    sysmem_local_omp_procs(*_sysmem_local_omp_procs),
    fbmem_local_procs(*_fbmem_local_procs),
    proc_sysmems(*_proc_sysmems),
    proc_regmems(*_proc_regmems),
    proc_fbmems(*_proc_fbmems),
    proc_zcmems(*_proc_zcmems)
{
  use_gpu = toc_procs_list.size() > 0;
  use_omp = omp_procs_list.size() > 0;
}

//--------------------------------------------------------------------------
Color SoleilMapper::get_task_color(MapperContext ctx, const Task &task,
                                   unsigned idx)
//--------------------------------------------------------------------------
{
  //assert(task.regions[0].handle_type == SINGULAR);

  const LogicalRegion& region = task.regions[idx].region;
  Color color = -1U;
  if (runtime->has_parent_index_partition(ctx, region.get_index_space())) {
    IndexPartition ip =
      runtime->get_parent_index_partition(ctx, region.get_index_space());
    Domain domain =
      runtime->get_index_partition_color_space(ctx, ip);
    DomainPoint point =
      runtime->get_logical_region_color_point(ctx, region);
    assert(domain.dim == 3);
    assert(point.dim == 3);
    coord_t size_x = domain.rect_data[3] - domain.rect_data[0] + 1;
    coord_t size_y = domain.rect_data[4] - domain.rect_data[1] + 1;
    color = point.point_data[0] +
            point.point_data[1] * size_x +
            point.point_data[2] * size_x * size_y;
  }

  return color;
}

//--------------------------------------------------------------------------
Processor SoleilMapper::default_policy_select_initial_processor(
                                    MapperContext ctx, const Task &task)
//--------------------------------------------------------------------------
{
  if (!task.regions.empty()) {
    if (task.regions[0].handle_type == SINGULAR &&
        task.regions[0].privilege != NO_ACCESS) {
      Color color = get_task_color(ctx, task);
      assert(color != -1U);

      // Special cases (which don't follow the default prioritization
      // of processor kinds).
      const char* task_name = task.get_task_name();
      const char* prefix = "shard_";
      if (strncmp(task_name, prefix, strlen(prefix)) == 0) {
          return io_procs_list[color % io_procs_list.size()];
      }

      VariantInfo info =
        default_find_preferred_variant(task, ctx, false/*needs tight*/);
      switch (info.proc_kind)
      {
        case Processor::LOC_PROC:
          return loc_procs_list[color % loc_procs_list.size()];
        case Processor::TOC_PROC:
          return toc_procs_list[color % toc_procs_list.size()];
        case Processor::OMP_PROC:
          return omp_procs_list[color % omp_procs_list.size()];
        default:
          break;
      }
    }
  }

  {
    // Special cases for tasks with no region arguments.
    const char* task_name = task.get_task_name();
    const char* prefix = "__";
    if (strncmp(task_name, prefix, strlen(prefix)) == 0) {
      return default_get_next_local_io();
    }
  }

  return DefaultMapper::default_policy_select_initial_processor(ctx, task);
}

//--------------------------------------------------------------------------
void SoleilMapper::default_policy_select_target_processors(
                                    MapperContext ctx,
                                    const Task &task,
                                    std::vector<Processor> &target_procs)
//--------------------------------------------------------------------------
{
  target_procs.push_back(task.target_proc);
}

LogicalRegion SoleilMapper::default_policy_select_instance_region(
                             MapperContext ctx, Memory target_memory,
                             const RegionRequirement &req,
                             const LayoutConstraintSet &layout_constraints,
                             bool force_new_instances,
                             bool meets_constraints)
{
  return req.region;
}

//--------------------------------------------------------------------------
void SoleilMapper::slice_task(const MapperContext      ctx,
                              const Task&              task,
                              const SliceTaskInput&    input,
                                    SliceTaskOutput&   output)
//--------------------------------------------------------------------------
{
  return DefaultMapper::slice_task(ctx, task, input, output);
}

//--------------------------------------------------------------------------
void SoleilMapper::map_task(const MapperContext      ctx,
                            const Task&              task,
                            const MapTaskInput&      input,
                                  MapTaskOutput&     output)
//--------------------------------------------------------------------------
{
  Processor::Kind target_kind = task.target_proc.kind();
  // Get the variant that we are going to use to map this task
  VariantInfo chosen = default_find_preferred_variant(task, ctx,
                    true/*needs tight bound*/, true/*cache*/, target_kind);
  output.chosen_variant = chosen.variant;
  // TODO: some criticality analysis to assign priorities
  output.task_priority = 0;
  output.postmap_task = false;
  // Figure out our target processors
  output.target_procs.push_back(task.target_proc);

  if (strcmp(task.get_task_name(), "Render") == 0)
  {
    Color color = get_task_color(ctx, task);
    assert(color != -1U);

    assert(task.regions.size() >= 3 && task.regions[0].handle_type == SINGULAR);
    std::pair<Color, RegionTreeID> key(color, task.regions[0].region.get_tree_id());

    std::map<std::pair<Color, RegionTreeID>, CachedTaskMapping>::iterator finder =
      mapping_cache_render.find(key);
    if (finder != mapping_cache_render.end())
    {
      CachedTaskMapping& cache = finder->second;
      bool ok =
        runtime->acquire_and_filter_instances(ctx, cache);
      if (!ok)
        fprintf(stderr, "cached mapping failed\n");
      assert(ok);
      output.chosen_instances = cache;
    }
    else
    {
      CachedTaskMapping& cache = mapping_cache_render[key];
      cache.resize(task.regions.size());
      Memory target_memory = proc_sysmems[task.target_proc];
      for (size_t idx = 0; idx < 2; ++idx)
      {
        PhysicalInstance inst;
        std::vector<LogicalRegion> target_region;
        target_region.push_back(task.regions[idx].region);
        LayoutConstraintSet constraints;
        std::vector<DimensionKind> dimension_ordering(4);
        dimension_ordering[0] = DIM_X;
        dimension_ordering[1] = DIM_Y;
        dimension_ordering[2] = DIM_Z;
        dimension_ordering[3] = DIM_F;
        constraints.add_constraint(MemoryConstraint(target_memory.kind()))
          .add_constraint(FieldConstraint(
                task.regions[idx].instance_fields, false, false))
          .add_constraint(OrderingConstraint(dimension_ordering, false));
        if(!runtime->create_physical_instance(ctx, target_memory,
              constraints, target_region, inst))
        {
          default_report_failed_instance_creation(task, idx,
              task.target_proc, target_memory);
        }
        runtime->set_garbage_collection_priority(ctx, inst, GC_NEVER_PRIORITY);
        cache[idx].push_back(inst);
      }
      for (size_t idx = 2; idx < task.regions.size(); ++idx)
      {
        cache[idx] = input.valid_instances[idx];
        assert(cache[idx].size() > 0);
        bool ok = runtime->acquire_instances(ctx, cache[idx]);
        if (!ok) fprintf(stderr, "failed to acquire instances\n");
      }

      output.chosen_instances = cache;

      //for (size_t idx = 0; idx < task.regions.size(); ++idx)
      //  fprintf(stderr, "task %s color %u reg (%d,%d,%d) processor %llx instance %lx memory %llx\n",
      //      task.get_task_name(), color,
      //      task.regions[idx].region.get_index_space().get_id(),
      //      task.regions[idx].region.get_field_space().get_id(),
      //      task.regions[idx].region.get_tree_id(),
      //      task.target_proc.id,
      //      output.chosen_instances[idx][0].get_instance_id(),
      //      output.chosen_instances[idx][0].get_location().id);
    }
    return;
  }

  if (task.parent_task != NULL && task.parent_task->must_epoch_task)
  {
    for (unsigned idx = 0; idx < task.regions.size(); idx++)
    {
      const RegionRequirement &req = task.regions[idx];

      // Skip any empty regions
      if ((req.privilege == NO_ACCESS) || (req.privilege_fields.empty()))
        continue;

      assert(input.valid_instances[idx].size() == 1);
      output.chosen_instances[idx] = input.valid_instances[idx];
      bool ok = runtime->acquire_and_filter_instances(ctx, output.chosen_instances);
      if (!ok) fprintf(stderr, "failed to acquire instances\n");
    }
    return;
  }

  // Now we need to go through and make instances for any of our
  // regions which do not have space for certain fields
  for (unsigned idx = 0; idx < task.regions.size(); idx++)
  {
    // Skip any empty regions
    if ((task.regions[idx].privilege == NO_ACCESS) ||
        (task.regions[idx].privilege_fields.empty()))
      continue;

    Memory target_memory = Memory::NO_MEMORY;
    bool is_pull_task = strcmp(task.get_task_name(), "pullAll") == 0;

    if (task.must_epoch_task || (!task.is_index_space && !is_pull_task))
    {
      if (task.target_proc.kind() == Processor::IO_PROC ||
          task.target_proc.kind() == Processor::LOC_PROC ||
          task.target_proc.kind() == Processor::OMP_PROC)
      {
        target_memory = proc_sysmems[task.target_proc];
        if (!runtime->has_parent_logical_partition(ctx, task.regions[idx].region))
        {
          std::map<Processor, Memory>::iterator finder = proc_regmems.find(task.target_proc);
          if (finder != proc_regmems.end()) target_memory = finder->second;
        }
      }
      else if (task.target_proc.kind() == Processor::TOC_PROC) {
        target_memory = proc_fbmems[task.target_proc];
        if (!runtime->has_parent_logical_partition(ctx, task.regions[idx].region))
        {
          std::map<Processor, Memory>::iterator finder = proc_zcmems.find(task.target_proc);
          if (finder != proc_zcmems.end()) target_memory = finder->second;
        }
      }
      else {
        assert(false);
      }
    }
    else if (is_pull_task)
    {
      if (task.target_proc.kind() == Processor::IO_PROC ||
          task.target_proc.kind() == Processor::LOC_PROC)
      {
        target_memory = proc_sysmems[task.target_proc];
        if (idx > 0)
        {
          std::map<Processor, Memory>::iterator finder = proc_regmems.find(task.target_proc);
          if (finder != proc_regmems.end()) target_memory = finder->second;
        }
      }
      else if (task.target_proc.kind() == Processor::TOC_PROC)
      {
        target_memory = proc_fbmems[task.target_proc];
        if (idx > 0)
        {
          std::map<Processor, Memory>::iterator finder = proc_zcmems.find(task.target_proc);
          if (finder != proc_zcmems.end()) target_memory = finder->second;
        }
      }
      else {
        assert(false);
      }
    }
    else
    {
      if (task.target_proc.kind() == Processor::IO_PROC ||
          task.target_proc.kind() == Processor::LOC_PROC ||
          task.target_proc.kind() == Processor::OMP_PROC)
      {
        target_memory = proc_sysmems[task.target_proc];
        if (task.regions.size() > 3 && idx >= 3)
        {
          std::map<Processor, Memory>::iterator finder = proc_regmems.find(task.target_proc);
          if (finder != proc_regmems.end()) target_memory = finder->second;
        }
      }
      else if (task.target_proc.kind() == Processor::TOC_PROC)
      {
        target_memory = proc_fbmems[task.target_proc];
        if (task.regions.size() > 3 && idx >= 3)
        {
          std::map<Processor, Memory>::iterator finder = proc_zcmems.find(task.target_proc);
          if (finder != proc_zcmems.end()) target_memory = finder->second;
        }
      }
      else {
        assert(false);
      }
    }
    assert(target_memory.exists());

    // Otherwise make normal instances for the given region
    {
      PhysicalInstance inst;
      std::vector<LogicalRegion> target_region;
      target_region.push_back(task.regions[idx].region);
      LayoutConstraintSet constraints;
      std::vector<DimensionKind> dimension_ordering(4);
      dimension_ordering[0] = DIM_X;
      dimension_ordering[1] = DIM_Y;
      dimension_ordering[2] = DIM_Z;
      dimension_ordering[3] = DIM_F;
      constraints.add_constraint(MemoryConstraint(target_memory.kind()))
        .add_constraint(FieldConstraint(
              task.regions[idx].instance_fields, false, false))
        .add_constraint(OrderingConstraint(dimension_ordering, false));
      bool created;
      if(!runtime->find_or_create_physical_instance(ctx, target_memory,
            constraints, target_region, inst, created))
      {
        default_report_failed_instance_creation(task, idx,
            task.target_proc, target_memory);
      }
      runtime->set_garbage_collection_priority(ctx, inst, GC_NEVER_PRIORITY);
      output.chosen_instances[idx].push_back(inst);
    }
  }
  //if (task.must_epoch_task)
  //{
  //  Color color = -1U;
  //  for (size_t idx = 0; idx < task.regions.size(); ++idx)
  //  {
  //    if ((task.regions[idx].privilege == NO_ACCESS) ||
  //        (task.regions[idx].privilege_fields.empty()) ||
  //        (task.regions[idx].is_no_access()))
  //      continue;
  //    color = get_task_color(ctx, task, idx);
  //    break;
  //  }
  //  assert(color != -1U);
  //  for (size_t idx = 0; idx < task.regions.size(); ++idx)
  //  {
  //    if ((task.regions[idx].privilege == NO_ACCESS) ||
  //        (task.regions[idx].privilege_fields.empty()) ||
  //        (task.regions[idx].is_no_access()))
  //      continue;
  //    fprintf(stderr, "task %s color %u reg %lu (%d,%d,%d) color %u processor %llx instance %lx memory %llx\n",
  //        task.get_task_name(), color, idx,
  //        task.regions[idx].region.get_index_space().get_id(),
  //        task.regions[idx].region.get_field_space().get_id(),
  //        task.regions[idx].region.get_tree_id(),
  //        get_task_color(ctx, task, idx),
  //        task.target_proc.id,
  //        output.chosen_instances[idx][0].get_instance_id(),
  //        output.chosen_instances[idx][0].get_location().id);
  //  }
  //}
}

//--------------------------------------------------------------------------
void SoleilMapper::map_copy(const MapperContext ctx,
                             const Copy &copy,
                             const MapCopyInput &input,
                             MapCopyOutput &output)
//--------------------------------------------------------------------------
{
  log_soleil.spew("Soleil mapper map_copy");
  for (unsigned idx = 0; idx < copy.src_requirements.size(); idx++)
  {
    // Always use a virtual instance for the source.
    output.src_instances[idx].clear();
    output.src_instances[idx].push_back(
      PhysicalInstance::get_virtual_instance());

    // Place the destination instance on the remote node.
    output.dst_instances[idx].clear();
    if (!copy.dst_requirements[idx].is_restricted()) {
      // Call a customized method to create an instance on the desired node.
      soleil_create_copy_instance<false/*is src*/>(ctx, copy,
        copy.dst_requirements[idx], idx, output.dst_instances[idx]);
    } else {
      // If it's restricted, just take the instance. This will only
      // happen inside the shard task.
      output.dst_instances[idx] = input.dst_instances[idx];
      if (!output.dst_instances[idx].empty())
        runtime->acquire_and_filter_instances(ctx,
                                output.dst_instances[idx]);
    }
  }
}

//--------------------------------------------------------------------------
bool SoleilMapper::soleil_create_custom_instances(MapperContext ctx,
                      Processor target_proc, Memory target_memory,
                      const RegionRequirement &req, unsigned index,
                      std::set<FieldID> &needed_fields,
                      const TaskLayoutConstraintSet &layout_constraints,
                      bool needs_field_constraint_check,
                      std::vector<PhysicalInstance> &instances)
//--------------------------------------------------------------------------
{
  // Before we do anything else figure out our
  // constraints for any instances of this task, then we'll
  // see if these constraints conflict with or are satisfied by
  // any of the other constraints
  bool force_new_instances = false;
  LayoutConstraintID our_layout_id =
   default_policy_select_layout_constraints(ctx, target_memory, req,
           TASK_MAPPING, needs_field_constraint_check, force_new_instances);
  const LayoutConstraintSet &our_constraints =
                runtime->find_layout_constraints(ctx, our_layout_id);

  instances.resize(instances.size()+1);
  LayoutConstraintSet creation_constraints = our_constraints;
  if (!default_make_instance(ctx, target_memory, creation_constraints,
            instances.back(), TASK_MAPPING, force_new_instances,
            true/*meets*/,  req))
    return false;
  return true;
}

//--------------------------------------------------------------------------
template<bool IS_SRC>
void SoleilMapper::soleil_create_copy_instance(MapperContext ctx,
                     const Copy &copy, const RegionRequirement &req,
                     unsigned idx, std::vector<PhysicalInstance> &instances)
//--------------------------------------------------------------------------
{
  // This method is identical to the default version except that it
  // chooses an intelligent memory based on the destination of the
  // copy.

  // See if we have all the fields covered
  std::set<FieldID> missing_fields = req.privilege_fields;
  for (std::vector<PhysicalInstance>::const_iterator it =
        instances.begin(); it != instances.end(); it++)
  {
    it->remove_space_fields(missing_fields);
    if (missing_fields.empty())
      break;
  }
  if (missing_fields.empty())
    return;
  // If we still have fields, we need to make an instance
  // We clearly need to take a guess, let's see if we can find
  // one of our instances to use.


  const LogicalRegion& region = copy.src_requirements[idx].region;
  IndexPartition ip = runtime->get_parent_index_partition(ctx, region.get_index_space());
  Domain domain = runtime->get_index_partition_color_space(ctx, ip);
  DomainPoint point =
    runtime->get_logical_region_color_point(ctx, region);
  coord_t size_x = domain.rect_data[3] - domain.rect_data[0] + 1;
  coord_t size_y = domain.rect_data[4] - domain.rect_data[1] + 1;
  Color color = point.point_data[0] +
                point.point_data[1] * size_x +
                point.point_data[2] * size_x * size_y;
  Memory target_memory = Memory::NO_MEMORY;
  if (use_gpu) {
    Processor proc = toc_procs_list[color % toc_procs_list.size()];
    target_memory = proc_fbmems[proc];
    std::map<Processor, Memory>::iterator finder = proc_zcmems.find(proc);
    if (finder != proc_zcmems.end()) target_memory = finder->second;
  }
  else if (use_omp) {
    Processor proc = omp_procs_list[color % omp_procs_list.size()];
    target_memory = proc_sysmems[proc];
    std::map<Processor, Memory>::iterator finder = proc_regmems.find(proc);
    if (finder != proc_regmems.end()) target_memory = finder->second;
  }
  else
  {
    Processor proc = loc_procs_list[color % loc_procs_list.size()];
    target_memory = proc_sysmems[proc];
    std::map<Processor, Memory>::iterator finder = proc_regmems.find(proc);
    if (finder != proc_regmems.end()) target_memory = finder->second;
  }
  assert(target_memory.exists());

  bool force_new_instances = false;
  LayoutConstraintSet creation_constraints;
  default_policy_select_constraints(ctx, creation_constraints, target_memory, req);
  creation_constraints.add_constraint(
      FieldConstraint(missing_fields,
                      false/*contig*/, false/*inorder*/));
  instances.resize(instances.size() + 1);
  if (!default_make_instance(ctx, target_memory,
        creation_constraints, instances.back(),
        COPY_MAPPING, force_new_instances, true/*meets*/, req))
  {
    // If we failed to make it that is bad
    log_soleil.error("Soleil mapper failed allocation for "
                     "%s region requirement %d of explicit "
                     "region-to-region copy operation in task %s "
                     "(ID %lld) in memory " IDFMT " for processor "
                     IDFMT ". This means the working set of your "
                     "application is too big for the allotted "
                     "capacity of the given memory under the default "
                     "mapper's mapping scheme. You have three "
                     "choices: ask Realm to allocate more memory, "
                     "write a custom mapper to better manage working "
                     "sets, or find a bigger machine. Good luck!",
                     IS_SRC ? "source" : "destination", idx,
                     copy.parent_task->get_task_name(),
                     copy.parent_task->get_unique_id(),
		                 target_memory.id,
		                 copy.parent_task->current_proc.id);
    assert(false);
  }
}

//--------------------------------------------------------------------------
void SoleilMapper::map_must_epoch(const MapperContext           ctx,
                                  const MapMustEpochInput&      input,
                                        MapMustEpochOutput&     output)
//--------------------------------------------------------------------------
{
  size_t num_nodes = use_gpu ? fbmems_list.size() : sysmems_list.size();
  size_t num_tasks = input.tasks.size();
  size_t num_shards_per_node =
    num_nodes < num_tasks ? (num_tasks + num_nodes - 1) / num_nodes : 1;
  bool use_io_procs = not use_gpu && sysmem_local_io_procs.size() > 0;
  std::map<const Task*, size_t> task_indices;

  for (size_t idx = 0; idx < num_tasks; ++idx) {
    Color color = -1U;
    size_t num_leaf_tasks_per_node = 0;
    const Task* task = input.tasks[idx];
    for (size_t i = 0; i < input.tasks[idx]->regions.size(); ++i)
    {
      if ((task->regions[i].privilege == NO_ACCESS) ||
          (task->regions[i].privilege_fields.empty()) ||
          (task->regions[i].is_no_access()))
        continue;
      const LogicalRegion& region = task->regions[i].region;
      IndexPartition ip =
        runtime->get_parent_index_partition(ctx, region.get_index_space());
      Domain domain =
        runtime->get_index_partition_color_space(ctx, ip);
      num_leaf_tasks_per_node = domain.get_volume() / num_tasks;
      DomainPoint point =
        runtime->get_logical_region_color_point(ctx, region);
      assert(domain.dim == 3);
      assert(point.dim == 3);
      coord_t size_x = domain.rect_data[3] - domain.rect_data[0] + 1;
      coord_t size_y = domain.rect_data[4] - domain.rect_data[1] + 1;
      color = point.point_data[0] +
              point.point_data[1] * size_x +
              point.point_data[2] * size_x * size_y;
      break;
    }
    assert(color != -1U);
    assert(num_leaf_tasks_per_node != 0);
    size_t node_idx = (color / num_leaf_tasks_per_node) / num_shards_per_node;
    size_t proc_idx = (color / num_leaf_tasks_per_node) % num_shards_per_node;

    //fprintf(stderr, "color %u node idx %lu proc idx %lu\n",
    //    color, node_idx, proc_idx);

    if (use_gpu)
      output.task_processors[idx] = fbmem_local_procs[fbmems_list[node_idx]][proc_idx];
    else if (use_io_procs)
      output.task_processors[idx] = sysmem_local_io_procs[sysmems_list[node_idx]][proc_idx];
    else
      output.task_processors[idx] = sysmem_local_procs[sysmems_list[node_idx]][proc_idx];
    task_indices[input.tasks[idx]] = idx;
  }

  for (size_t idx = 0; idx < input.constraints.size(); ++idx) {
    const MappingConstraint& constraint = input.constraints[idx];
    int owner_id = -1;

    //fprintf(stderr, "Constraint %lu: \n", idx);
    for (unsigned i = 0; i < constraint.constrained_tasks.size(); ++i) {
      const RegionRequirement& req =
        constraint.constrained_tasks[i]->regions[
          constraint.requirement_indexes[i]];
      if (req.is_no_access()) continue;
      assert(owner_id == -1);
      owner_id = static_cast<int>(i);
      //{
      //  const Task* task = constraint.constrained_tasks[owner_id];
      //  const RegionRequirement& req =
      //    task->regions[constraint.requirement_indexes[owner_id]];
      //  fprintf(stderr, "   task %lu reg (%d,%d,%d)\n",
      //        task_indices[task],
      //        req.region.get_index_space().get_id(),
      //        req.region.get_field_space().get_id(),
      //        req.region.get_tree_id());
      //}
    }
    assert(owner_id != -1);

    const Task* task = constraint.constrained_tasks[owner_id];
    const RegionRequirement& req =
      task->regions[constraint.requirement_indexes[owner_id]];
    Processor task_proc = output.task_processors[task_indices[task]];
    Memory target_memory = use_gpu ? proc_fbmems[task_proc] : proc_sysmems[task_proc];
    if (!runtime->has_parent_logical_partition(ctx, req.region)) {
      if (!use_gpu) {
        std::map<Processor, Memory>::iterator finder = proc_regmems.find(task_proc);
        if (finder != proc_regmems.end()) target_memory = finder->second;
      }
      else {
        std::map<Processor, Memory>::iterator finder = proc_zcmems.find(task_proc);
        if (finder != proc_zcmems.end()) target_memory = finder->second;
      }
    }
    LayoutConstraintSet layout_constraints;
    default_policy_select_constraints(ctx, layout_constraints, target_memory, req);
    layout_constraints.add_constraint(
      FieldConstraint(req.privilege_fields, false /*!contiguous*/));

	  PhysicalInstance inst;
    bool created;
    bool ok = runtime->find_or_create_physical_instance(ctx, target_memory,
        layout_constraints, std::vector<LogicalRegion>(1, req.region),
        inst, created, true /*acquire*/);
    if(!ok) {
      log_soleil.fatal("Soleil mapper error. Unable to make instance(s) "
          "in memory " IDFMT " for index %d of constrained "
          "task %s (ID %lld) in must epoch launch.",
          target_memory.id, constraint.requirement_indexes[0],
          constraint.constrained_tasks[0]->get_task_name(),
          constraint.constrained_tasks[0]->get_unique_id());
      assert(false);
    }
    output.constraint_mappings[idx].push_back(inst);
    //fprintf(stderr, "instance %lx memory %llx\n",
    //    inst.get_instance_id(), target_memory.id);
  }
}

static void create_mappers(Machine machine,
                           HighLevelRuntime *runtime,
                           const std::set<Processor> &local_procs)
{
  std::vector<Processor>* loc_procs_list = new std::vector<Processor>();
  std::vector<Processor>* toc_procs_list = new std::vector<Processor>();
  std::vector<Processor>* io_procs_list = new std::vector<Processor>();
  std::vector<Processor>* omp_procs_list = new std::vector<Processor>();
  std::vector<Memory>* sysmems_list = new std::vector<Memory>();
  std::vector<Memory>* fbmems_list = new std::vector<Memory>();
  std::map<Memory, std::vector<Processor> >* sysmem_local_procs =
    new std::map<Memory, std::vector<Processor> >();
  std::map<Memory, std::vector<Processor> >* sysmem_local_io_procs =
    new std::map<Memory, std::vector<Processor> >();
  std::map<Memory, std::vector<Processor> >* sysmem_local_omp_procs =
    new std::map<Memory, std::vector<Processor> >();
  std::map<Memory, std::vector<Processor> >* fbmem_local_procs =
    new std::map<Memory, std::vector<Processor> >();
  std::map<Processor, Memory>* proc_sysmems = new std::map<Processor, Memory>();
  std::map<Processor, Memory>* proc_regmems = new std::map<Processor, Memory>();
  std::map<Processor, Memory>* proc_fbmems = new std::map<Processor, Memory>();
  std::map<Processor, Memory>* proc_zcmems = new std::map<Processor, Memory>();

  std::vector<Machine::ProcessorMemoryAffinity> proc_mem_affinities;
  machine.get_proc_mem_affinity(proc_mem_affinities);

  for (unsigned idx = 0; idx < proc_mem_affinities.size(); ++idx) {
    Machine::ProcessorMemoryAffinity& affinity = proc_mem_affinities[idx];
    if (affinity.p.kind() == Processor::LOC_PROC ||
       affinity.p.kind() == Processor::IO_PROC ||
       affinity.p.kind() == Processor::OMP_PROC) {
      if (affinity.m.kind() == Memory::SYSTEM_MEM) {
        (*proc_sysmems)[affinity.p] = affinity.m;
      }
      else if (affinity.m.kind() == Memory::REGDMA_MEM) {
        (*proc_regmems)[affinity.p] = affinity.m;
      }
    }
    else if (affinity.p.kind() == Processor::TOC_PROC) {
      if (affinity.m.kind() == Memory::GPU_FB_MEM) {
        (*proc_fbmems)[affinity.p] = affinity.m;
      }
      else if (affinity.m.kind() == Memory::Z_COPY_MEM) {
        (*proc_zcmems)[affinity.p] = affinity.m;
      }
    }
  }

  for (std::map<Processor, Memory>::iterator it = proc_sysmems->begin();
       it != proc_sysmems->end(); ++it) {
    if (it->first.kind() == Processor::LOC_PROC) {
      loc_procs_list->push_back(it->first);
      (*sysmem_local_procs)[it->second].push_back(it->first);
    }
    else if (it->first.kind() == Processor::IO_PROC) {
      io_procs_list->push_back(it->first);
      (*sysmem_local_io_procs)[it->second].push_back(it->first);
    }
    else if (it->first.kind() == Processor::OMP_PROC) {
      omp_procs_list->push_back(it->first);
      (*sysmem_local_omp_procs)[it->second].push_back(it->first);
    }
  }

  for (std::map<Memory, std::vector<Processor> >::iterator it =
        sysmem_local_procs->begin(); it != sysmem_local_procs->end(); ++it)
    sysmems_list->push_back(it->first);

  for (std::map<Processor, Memory>::iterator it = proc_fbmems->begin();
       it != proc_fbmems->end(); ++it) {
    if (it->first.kind() == Processor::TOC_PROC) {
      toc_procs_list->push_back(it->first);
      (*fbmem_local_procs)[it->second].push_back(it->first);
    }
  }

  for (std::map<Memory, std::vector<Processor> >::iterator it =
        fbmem_local_procs->begin(); it != fbmem_local_procs->end(); ++it)
    fbmems_list->push_back(it->first);

  for (std::set<Processor>::const_iterator it = local_procs.begin();
        it != local_procs.end(); it++)
  {
    SoleilMapper* mapper = new SoleilMapper(runtime->get_mapper_runtime(),
                                            machine, *it, "soleil_mapper",
                                            loc_procs_list,
                                            toc_procs_list,
                                            io_procs_list,
                                            omp_procs_list,
                                            sysmems_list,
                                            fbmems_list,
                                            sysmem_local_procs,
                                            sysmem_local_io_procs,
                                            sysmem_local_omp_procs,
                                            fbmem_local_procs,
                                            proc_sysmems,
                                            proc_regmems,
                                            proc_fbmems,
                                            proc_zcmems);
    runtime->replace_default_mapper(mapper, *it);
  }
}

void register_mappers()
{
  HighLevelRuntime::add_registration_callback(create_mappers);
}
