#include "internal.hpp"

namespace aegis::detail {

std::uint64_t CommandPipeline::enqueue(std::string name){const auto id=next_id_++;stages_.push_back({id,std::move(name),false});return id;}

bool CommandPipeline::complete(std::uint64_t id){const auto found=std::find_if(stages_.begin(),stages_.end(),[id](const auto&stage){return stage.id==id;});if(found==stages_.end())return false;found->completed=true;return true;}

std::vector<PipelineStage> CommandPipeline::pending() const {std::vector<PipelineStage> out;for(const auto&stage:stages_)if(!stage.completed)out.push_back(stage);return out;}

void CommandPipeline::clear(){stages_.clear();next_id_=1U;}

}  // namespace aegis::detail
