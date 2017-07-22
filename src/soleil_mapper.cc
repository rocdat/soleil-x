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

#define NUM_RENDER_PROCS 2

class SoleilMapper : public DefaultMapper
{
public:
  SoleilMapper(MapperRuntime *rt, Machine machine, Processor local,
                const char *mapper_name,
                std::vector<Processor>* loc_procs_list,
                std::vector<Processor>* toc_procs_list,
                std::vector<Memory>* sysmems_list,
                std::vector<Memory>* fbmems_list,
                std::map<Memory, std::vector<Processor> >* sysmem_local_procs,
                std::map<Memory, std::vector<Processor> >* sysmem_local_io_procs,
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
private:
  bool use_gpu;
  std::vector<Processor>& loc_procs_list;
  std::vector<Processor>& toc_procs_list;
  std::vector<Memory>& sysmems_list;
  std::vector<Memory>& fbmems_list;
  std::map<Memory, std::vector<Processor> >& sysmem_local_procs;
  std::map<Memory, std::vector<Processor> >& sysmem_local_io_procs;
  std::map<Memory, std::vector<Processor> >& fbmem_local_procs;
  std::map<Processor, Memory>& proc_sysmems;
  std::map<Processor, Memory>& proc_regmems;
  std::map<Processor, Memory>& proc_fbmems;
  std::map<Processor, Memory>& proc_zcmems;

  std::vector<TaskSlice> slice_cache_compute;
  std::vector<TaskSlice> slice_cache_render;
  typedef std::vector<PhysicalInstance> CachedRegionMapping;
  typedef std::vector<CachedRegionMapping> CachedTaskMapping;
  std::map<DomainPoint, CachedTaskMapping> mapping_cache_render;
  std::map<DomainPoint, CachedRegionMapping> mapping_cache_particle;
  std::map<DomainPoint, CachedTaskMapping> mapping_cache_init;
  std::map<DomainPoint, CachedTaskMapping> mapping_cache_push;
  std::map<DomainPoint, CachedTaskMapping> mapping_cache_pull;
};

//--------------------------------------------------------------------------
SoleilMapper::SoleilMapper(MapperRuntime *rt, Machine machine, Processor local,
                             const char *mapper_name,
                             std::vector<Processor>* _loc_procs_list,
                             std::vector<Processor>* _toc_procs_list,
                             std::vector<Memory>* _sysmems_list,
                             std::vector<Memory>* _fbmems_list,
                             std::map<Memory, std::vector<Processor> >* _sysmem_local_procs,
                             std::map<Memory, std::vector<Processor> >* _sysmem_local_io_procs,
                             std::map<Memory, std::vector<Processor> >* _fbmem_local_procs,
                             std::map<Processor, Memory>* _proc_sysmems,
                             std::map<Processor, Memory>* _proc_regmems,
                             std::map<Processor, Memory>* _proc_fbmems,
                             std::map<Processor, Memory>* _proc_zcmems)
//--------------------------------------------------------------------------
  : DefaultMapper(rt, machine, local, mapper_name),
    use_gpu(false),
    loc_procs_list(*_loc_procs_list),
    toc_procs_list(*_toc_procs_list),
    sysmems_list(*_sysmems_list),
    fbmems_list(*_fbmems_list),
    sysmem_local_procs(*_sysmem_local_procs),
    sysmem_local_io_procs(*_sysmem_local_io_procs),
    fbmem_local_procs(*_fbmem_local_procs),
    proc_sysmems(*_proc_sysmems),
    proc_regmems(*_proc_regmems),
    proc_fbmems(*_proc_fbmems),
    proc_zcmems(*_proc_zcmems)
{
  use_gpu = toc_procs_list.size() > 0;
}

//--------------------------------------------------------------------------
Processor SoleilMapper::default_policy_select_initial_processor(
                                    MapperContext ctx, const Task &task)
//--------------------------------------------------------------------------
{
  if (!task.regions.empty()) {
    if (task.regions[0].handle_type == SINGULAR) {
      const LogicalRegion& region = task.regions[0].region;
      Color color = 0;
      if (runtime->has_parent_index_partition(ctx, region.get_index_space())) {
        IndexPartition ip =
          runtime->get_parent_index_partition(ctx, region.get_index_space());
        Domain domain =
          runtime->get_index_partition_color_space(ctx, ip);
        DomainPoint point =
          runtime->get_logical_region_color_point(ctx, region);
        coord_t size_x = domain.rect_data[3] - domain.rect_data[0] + 1;
        coord_t size_y = domain.rect_data[4] - domain.rect_data[1] + 1;
        color = point.point_data[0] +
                point.point_data[1] * size_x +
                point.point_data[2] * size_x * size_y;
      }
      return use_gpu ? toc_procs_list[color % toc_procs_list.size()]
                     : loc_procs_list[color % loc_procs_list.size()];
    }
  }
  else if (strcmp(task.get_task_name(), "main") == 0)
  {
    std::vector<Processor>& proc_list = sysmem_local_procs[sysmems_list[0]];
    return proc_list.size() > NUM_RENDER_PROCS ?
           proc_list[NUM_RENDER_PROCS] : *proc_list.begin();
  }
  else if (std::string(task.get_task_name()).find("__binary") == 0)
  {
    std::vector<Processor>& proc_list =
      sysmem_local_procs[proc_sysmems[task.parent_task->target_proc]];
    return proc_list.size() > NUM_RENDER_PROCS ?
           proc_list[NUM_RENDER_PROCS] : *proc_list.begin();
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
  const char* task_name = task.get_task_name();
  assert(input.domain.dim == 3);
  Rect<3> dom_rect = input.domain.get_rect<3>();

  if (use_gpu)
  {
    if (strcmp(task_name, "Render") == 0 || strcmp(task_name, "Reduce") == 0)
    {
      if (slice_cache_render.size() == 0)
      {
        Point<3> num_blocks =
          default_select_num_blocks<3>(loc_procs_list.size(), dom_rect);
        default_decompose_points<3>(dom_rect, loc_procs_list,
            num_blocks, false, stealing_enabled, slice_cache_render);
      }
      output.slices = slice_cache_render;
    }
    else
    {
      if (slice_cache_compute.size() == 0)
      {
        Point<3> num_blocks =
          default_select_num_blocks<3>(toc_procs_list.size(), dom_rect);
        default_decompose_points<3>(dom_rect, toc_procs_list,
            num_blocks, false, stealing_enabled, slice_cache_compute);
      }
      output.slices = slice_cache_compute;
    }
  }
  else
  {
    if (NUM_RENDER_PROCS > 0 &&
        sysmem_local_procs.begin()->second.size() > NUM_RENDER_PROCS &&
        (strcmp(task_name, "Render") == 0 || strcmp(task_name, "Reduce") == 0))
    {
      if (slice_cache_render.size() == 0)
      {
        std::vector<Processor> render_procs;
        for (std::map<Memory, std::vector<Processor> >::iterator
            it = sysmem_local_procs.begin(); it != sysmem_local_procs.end();
            ++it)
        {
          for (unsigned idx = 0; idx < NUM_RENDER_PROCS; ++idx)
            render_procs.push_back(it->second[idx]);
        }
        Point<3> num_blocks =
          default_select_num_blocks<3>(render_procs.size(), dom_rect);
        default_decompose_points<3>(dom_rect, render_procs,
            num_blocks, false, stealing_enabled, slice_cache_render);
      }
      output.slices = slice_cache_render;
    }
    else
    {
      if (slice_cache_compute.size() == 0)
      {
        std::vector<Processor> compute_procs;
        for (std::map<Memory, std::vector<Processor> >::iterator
            it = sysmem_local_procs.begin(); it != sysmem_local_procs.end();
            ++it)
        {
          for (unsigned idx = NUM_RENDER_PROCS; idx < it->second.size(); ++idx)
            compute_procs.push_back(it->second[idx]);
        }
        Point<3> num_blocks =
          default_select_num_blocks<3>(compute_procs.size(), dom_rect);
        default_decompose_points<3>(dom_rect, compute_procs,
            num_blocks, false, stealing_enabled, slice_cache_compute);
      }
      output.slices = slice_cache_compute;
    }
  }
}

enum TaskKind
{
  TASK_INIT = 1,
  TASK_PUSH,
  TASK_PULL,
  TASK_OTHERS,
};

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
  default_policy_select_target_processors(ctx, task, output.target_procs);

  // See if we have an inner variant, if we do virtually map all the regions
  // We don't even both caching these since they are so simple
  if (chosen.is_inner)
  {
    // Check to see if we have any relaxed coherence modes in which
    // case we can no longer do virtual mappings so we'll fall through
    bool has_relaxed_coherence = false;
    for (unsigned idx = 0; idx < task.regions.size(); idx++)
    {
      if (task.regions[idx].prop != EXCLUSIVE)
      {
        has_relaxed_coherence = true;
        break;
      }
    }
    if (!has_relaxed_coherence)
    {
      std::vector<unsigned> reduction_indexes;
      for (unsigned idx = 0; idx < task.regions.size(); idx++)
      {
        // As long as this isn't a reduction-only region requirement
        // we will do a virtual mapping, for reduction-only instances
        // we will actually make a physical instance because the runtime
        // doesn't allow virtual mappings for reduction-only privileges
        if (task.regions[idx].privilege == REDUCE)
          reduction_indexes.push_back(idx);
        else
          output.chosen_instances[idx].push_back(
              PhysicalInstance::get_virtual_instance());
      }
      if (!reduction_indexes.empty())
      {
        const TaskLayoutConstraintSet &layout_constraints =
            runtime->find_task_layout_constraints(ctx,
                                  task.task_id, output.chosen_variant);
        Memory target_memory = default_policy_select_target_memory(ctx,
                                                     task.target_proc);
        for (std::vector<unsigned>::const_iterator it =
              reduction_indexes.begin(); it !=
              reduction_indexes.end(); it++)
        {
          std::set<FieldID> copy = task.regions[*it].privilege_fields;
          if (!soleil_create_custom_instances(ctx, task.target_proc,
              target_memory, task.regions[*it], *it, copy,
              layout_constraints, false/*needs constraint check*/,
              output.chosen_instances[*it]))
          {
            default_report_failed_instance_creation(task, *it,
                                        task.target_proc, target_memory);
          }
        }
      }
      return;
    }
  }

  if (strcmp(task.get_task_name(), "Render") == 0)
  {
    std::map<DomainPoint, CachedTaskMapping>::iterator finder =
      mapping_cache_render.find(task.index_point);
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
      CachedTaskMapping& cache = mapping_cache_render[task.index_point];
      cache.resize(task.regions.size());
      Memory target_memory = proc_sysmems[task.target_proc];
      for (size_t idx = 0; idx < task.regions.size(); ++idx)
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
        runtime->create_physical_instance(ctx, target_memory,
              constraints, target_region, inst);
        runtime->set_garbage_collection_priority(ctx, inst, GC_NEVER_PRIORITY);
        cache[idx].push_back(inst);
      }

      output.chosen_instances = cache;
    }

    return;
  }

  if (strcmp(task.get_task_name(), "SaveImage") == 0)
  {
    Memory target_memory = proc_sysmems[task.target_proc];
    for (size_t idx = 0; idx < task.regions.size(); ++idx)
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
      runtime->find_or_create_physical_instance(ctx, target_memory,
            constraints, target_region, inst, created);
      runtime->set_garbage_collection_priority(ctx, inst, GC_FIRST_PRIORITY);
      output.chosen_instances[idx].push_back(inst);
    }
    return;
  }

  TaskKind kind =
    strcmp(task.get_task_name(), "InitParticlesUniform.parallelized") == 0 ?
    TASK_INIT : (strcmp(task.get_task_name(), "pushAll") == 0 ?
      TASK_PUSH : (strcmp(task.get_task_name(), "pullAll") == 0 ?
        TASK_PULL : TASK_OTHERS));

  if (kind != TASK_OTHERS)
  {
    // Check the cache for particle region
    {
      assert(task.regions.size() > 0);
      std::map<DomainPoint, CachedRegionMapping>::iterator finder =
        mapping_cache_particle.find(task.index_point);
      if (finder != mapping_cache_particle.end())
      {
        CachedRegionMapping& cache = finder->second;
        bool ok = runtime->acquire_and_filter_instances(ctx, cache);
        if (!ok) fprintf(stderr, "cached mapping failed\n");
        assert(ok);
        output.chosen_instances[0] = cache;
      }
      else
      {
        CachedRegionMapping& cache = mapping_cache_particle[task.index_point];
        Memory target_memory = proc_sysmems[task.target_proc];
        PhysicalInstance inst;
        std::vector<LogicalRegion> target_region;
        target_region.push_back(task.regions[0].region);
        LayoutConstraintSet constraints;
        std::vector<DimensionKind> dimension_ordering(4);
        dimension_ordering[0] = DIM_X;
        dimension_ordering[1] = DIM_Y;
        dimension_ordering[2] = DIM_Z;
        dimension_ordering[3] = DIM_F;
        constraints.add_constraint(MemoryConstraint(target_memory.kind()))
          .add_constraint(FieldConstraint(
                task.regions[0].instance_fields, false, false))
          .add_constraint(OrderingConstraint(dimension_ordering, false));
        runtime->create_physical_instance(ctx, target_memory,
              constraints, target_region, inst);
        runtime->set_garbage_collection_priority(ctx, inst, GC_NEVER_PRIORITY);
        cache.push_back(inst);

        output.chosen_instances[0] = cache;
      }
    }

    // Check the cache for the rest of region requirements
    {
      std::map<DomainPoint, CachedTaskMapping>& mapping_cache =
      kind == TASK_INIT ? mapping_cache_init :
        (kind == TASK_PUSH ? mapping_cache_push : mapping_cache_pull);
      std::map<DomainPoint, CachedTaskMapping>::iterator finder =
        mapping_cache.find(task.index_point);
      if (finder != mapping_cache.end())
      {
        CachedTaskMapping& cache = finder->second;
        bool ok =
          runtime->acquire_and_filter_instances(ctx, cache);
        if (!ok)
          fprintf(stderr, "cached mapping failed\n");
        assert(ok);
        for (size_t idx = 1; idx < task.regions.size(); ++idx)
          output.chosen_instances[idx] = cache[idx];
      }
      else
      {
        CachedTaskMapping& cache = mapping_cache[task.index_point];
        cache.resize(task.regions.size());
        Memory target_memory = proc_sysmems[task.target_proc];
        for (size_t idx = 1; idx < task.regions.size(); ++idx)
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
          runtime->find_or_create_physical_instance(ctx, target_memory,
                constraints, target_region, inst, created);
          runtime->set_garbage_collection_priority(ctx, inst, GC_NEVER_PRIORITY);
          cache[idx].push_back(inst);
          output.chosen_instances[idx] = cache[idx];
        }
      }
    }

    return;
  }

  bool needs_field_constraint_check = false;

  const TaskLayoutConstraintSet &layout_constraints =
    runtime->find_task_layout_constraints(ctx,
                          task.task_id, output.chosen_variant);
  // Now we need to go through and make instances for any of our
  // regions which do not have space for certain fields
  //bool has_reductions = false;
  //fprintf(stderr, "task %s\n", task.get_task_name());
  for (unsigned idx = 0; idx < task.regions.size(); idx++)
  {
    // Skip any empty regions
    if ((task.regions[idx].privilege == NO_ACCESS) ||
        (task.regions[idx].privilege_fields.empty()))
      continue;

    Memory target_memory = Memory::NO_MEMORY;
    bool is_pull_task = kind == TASK_PULL;

    if (task.must_epoch_task || (!task.is_index_space && !is_pull_task))
    {
      if (task.target_proc.kind() == Processor::IO_PROC || task.target_proc.kind() == Processor::LOC_PROC)
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
      if (task.target_proc.kind() == Processor::IO_PROC || task.target_proc.kind() == Processor::LOC_PROC)
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
      if (task.target_proc.kind() == Processor::IO_PROC || task.target_proc.kind() == Processor::LOC_PROC)
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

    std::set<FieldID> missing_fields(task.regions[idx].privilege_fields);
    // See if this is a reduction
    if (task.regions[idx].privilege == REDUCE)
    {
      if (!soleil_create_custom_instances(ctx, task.target_proc,
              target_memory, task.regions[idx], idx, missing_fields,
              layout_constraints, needs_field_constraint_check,
              output.chosen_instances[idx]))
      {
        default_report_failed_instance_creation(task, idx,
                                    task.target_proc, target_memory);
      }
      continue;
    }
    // Did the application request a virtual mapping for this requirement?
    if ((task.regions[idx].tag & DefaultMapper::VIRTUAL_MAP) != 0)
    {
      PhysicalInstance virt_inst = PhysicalInstance::get_virtual_instance();
      output.chosen_instances[idx].push_back(virt_inst);
      continue;
    }
    // Otherwise make normal instances for the given region
    if (!soleil_create_custom_instances(ctx, task.target_proc,
            target_memory, task.regions[idx], idx, missing_fields,
            layout_constraints, needs_field_constraint_check,
            output.chosen_instances[idx]))
    {
      default_report_failed_instance_creation(task, idx,
                                  task.target_proc, target_memory);
    }
  }
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
  if (!use_gpu) {
    Processor proc = loc_procs_list[color % loc_procs_list.size()];
    target_memory = proc_sysmems[proc];
    std::map<Processor, Memory>::iterator finder = proc_regmems.find(proc);
    if (finder != proc_regmems.end()) target_memory = finder->second;
  }
  else {
    Processor proc = toc_procs_list[color % toc_procs_list.size()];
    target_memory = proc_fbmems[proc];
    std::map<Processor, Memory>::iterator finder = proc_zcmems.find(proc);
    if (finder != proc_zcmems.end()) target_memory = finder->second;
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
    num_nodes < input.tasks.size() ? (num_tasks + num_nodes - 1) / num_nodes : 1;
  bool use_io_procs = not use_gpu && sysmem_local_io_procs.size() > 0;
  std::map<const Task*, size_t> task_indices;
  for (size_t idx = 0; idx < input.tasks.size(); ++idx) {
    size_t node_idx = idx / num_shards_per_node;
    size_t proc_idx = idx % num_shards_per_node;
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

    for (unsigned i = 0; i < constraint.constrained_tasks.size(); ++i) {
      const RegionRequirement& req =
        constraint.constrained_tasks[i]->regions[
          constraint.requirement_indexes[i]];
      if (req.is_no_access()) continue;
      assert(owner_id == -1);
      owner_id = static_cast<int>(i);
    }
    assert(owner_id != -1);

    const Task* task = constraint.constrained_tasks[owner_id];
    const RegionRequirement& req =
      task->regions[constraint.requirement_indexes[owner_id]];
    Memory target_memory = use_gpu ? fbmems_list[task_indices[task] / num_shards_per_node]
                                   : sysmems_list[task_indices[task] / num_shards_per_node];
    if (!runtime->has_parent_logical_partition(ctx, req.region)) {
      Processor task_proc = output.task_processors[task_indices[task]];
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
  }
}

static void create_mappers(Machine machine,
                           HighLevelRuntime *runtime,
                           const std::set<Processor> &local_procs)
{
  std::vector<Processor>* loc_procs_list = new std::vector<Processor>();
  std::vector<Processor>* toc_procs_list = new std::vector<Processor>();
  std::vector<Memory>* sysmems_list = new std::vector<Memory>();
  std::vector<Memory>* fbmems_list = new std::vector<Memory>();
  std::map<Memory, std::vector<Processor> >* sysmem_local_procs =
    new std::map<Memory, std::vector<Processor> >();
  std::map<Memory, std::vector<Processor> >* sysmem_local_io_procs =
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
       affinity.p.kind() == Processor::IO_PROC) {
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
      (*sysmem_local_io_procs)[it->second].push_back(it->first);
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
                                            sysmems_list,
                                            fbmems_list,
                                            sysmem_local_procs,
                                            sysmem_local_io_procs,
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
